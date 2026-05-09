#include "bsp.h"
#include "MenuSys.h"
#include "eeprom_metadata.h"
#include "freq_snr_test.h"

/*============================ 全局状态 ============================*/
volatile uint8_t g_current_state = STATE_COUNT;   // 无效态，确保首次状态切换生效
uint8_t g_playback_mode = PLAY_MODE_NORMAL;       // 播放模式（正常/倒放）
uint32_t g_current_rec_sample_rate_hz = 16000;    // 录音采样率（16kHz / 32kHz）
uint8_t g_playspeed_mode = 1;                     // 播放速度倍率（1=正常, 2=2倍速）
uint16_t s_global_adc_min = 0;                    // 全局 ADC 最小值 (录音时追踪)
uint16_t s_global_adc_max = 4095;                 // 全局 ADC 最大值 (默认满量程, 不拉伸)

/*============================ 显示节流变量 ============================*/
static uint32_t s_rec_last_sec  = 0xFFFFFFFF;     // 录音秒数缓存
static uint32_t s_play_last_sec = 0xFFFFFFFF;     // 播放秒数缓存

/*============================ 计块法播放控制 ============================*/
// 录制时统计半缓冲块数，播放时严格播完同等块数后停止
static uint32_t s_total_play_blocks;               // 录制总块数
static uint32_t s_blocks_played;                   // 已播完块数
static uint32_t s_blocks_fed;                      // 已从 Flash 读入的块数
static bool s_bypass_running;
static bool s_bypass_dac_started;

#define FILTER_WRITE_CHUNK_SAMPLES   (128U)
static uint16_t s_filter_write_buffer[FILTER_WRITE_CHUNK_SAMPLES];
static uint16_t s_raw_temp[INTERP_CHUNK_RAW_SIZE];

/*============================ 辅助函数 ============================*/

// 阻塞等待任意按键
static void WaitForKey(void)
{
    while (GetKeyNum_NonBlocking() == 0);
    delay_ms(200);
}

// 反转样本顺序（用于倒放）
static void ReverseSamples(volatile uint16_t *buf, uint16_t count)
{
    uint16_t i, tmp;
    for (i = 0; i < count / 2; i++)
    {
        tmp = buf[i];
        buf[i] = buf[count - 1 - i];
        buf[count - 1 - i] = tmp;
    }
}

/*
 * 线性插值 2x: raw[0..raw_count-1] → interp[0..2*raw_count-1]
 *   interp[2n]   = raw[n]               // 原始值
 *   interp[2n+1] = (raw[n]+raw[n+1])/2  // 两邻点均值
 *   末尾点: interp[2*raw_count-1] = raw[raw_count-1]     (hold)
 */
static void InterpolateLinear(const volatile uint16_t *raw,
                               volatile uint16_t *interp,
                               uint16_t raw_count)
{
    uint16_t n;
    for (n = 0; n < raw_count; n++) {
        interp[n * 2] = raw[n];
        if (n + 1 < raw_count) {
            interp[n * 2 + 1] = (raw[n] + raw[n + 1]) >> 1;
        } else {
            interp[n * 2 + 1] = raw[n];
        }
    }
}

/*
 * 从 Flash 读 raw 块 → 全局归一化拉伸 → 线性插值 → 填入 dac_buffer
 *
 * 健壮性设计:
 *   range < NORM_SILENCE_RANGE → 静音, 输出全 0
 *   range > NORM_HIGH_RANGE → 信号已满, 不拉伸 (unity gain)
 *   gain 上限 NORM_MAX_GAIN → 防止弱信号底噪被过度放大
 *   Q12 定点, 32-bit 乘积安全
 */
static void FillInterpolatedHalf(uint32_t rawFlashAddr,
                                  uint8_t half_idx,
                                  bool reverse)
{
    volatile uint16_t *target = &dac_buffer[half_idx * BUFFER_HALF_SIZE];
    uint16_t range = s_global_adc_max - s_global_adc_min;
    uint32_t gain_q12;
    uint16_t n;

    W25Q64_ReadData(rawFlashAddr, (uint8_t *)s_raw_temp,
                    INTERP_CHUNK_RAW_SIZE * sizeof(uint16_t));

    if (range <= NORM_SILENCE_RANGE) {
        for (n = 0; n < BUFFER_HALF_SIZE; n++) target[n] = 0;
        return;
    }

    if (range >= NORM_HIGH_RANGE) {
        gain_q12 = (uint32_t)1u << 12;
    } else {
        gain_q12 = ((uint32_t)4095u << 12) / range;
        if (gain_q12 > ((uint32_t)NORM_MAX_GAIN << 12)) {
            gain_q12 = (uint32_t)NORM_MAX_GAIN << 12;
        }
    }

    /* 拉伸 + 插值, 一笔循环完成 */
    {
        int32_t raw_val;
        int32_t shifted;
        uint16_t prev_exp;

        raw_val = ((int32_t)s_raw_temp[0] - (int32_t)s_global_adc_min) * (int32_t)gain_q12;
        shifted = raw_val >> 12;
        if (shifted < 0) shifted = 0;
        if (shifted > 4095) shifted = 4095;
        prev_exp = (uint16_t)shifted;
        target[0] = prev_exp;

        for (n = 1; n < INTERP_CHUNK_RAW_SIZE; n++) {
            uint16_t curr_exp;

            raw_val = ((int32_t)s_raw_temp[n] - (int32_t)s_global_adc_min) * (int32_t)gain_q12;
            shifted = raw_val >> 12;
            if (shifted < 0) shifted = 0;
            if (shifted > 4095) shifted = 4095;
            curr_exp = (uint16_t)shifted;

            target[n * 2 - 1] = (prev_exp + curr_exp) >> 1;
            target[n * 2]     = curr_exp;
            prev_exp = curr_exp;
        }
        target[INTERP_CHUNK_RAW_SIZE * 2 - 1] = prev_exp;
    }

    if (reverse) {
        ReverseSamples(target, BUFFER_HALF_SIZE);
    }
}

// 清零指定半缓冲区（0=前半, 1=后半）
static void ClearHalfBuffer(uint8_t half_idx)
{
    uint16_t i;
    if (half_idx == 0)
    {
        for (i = 0; i < BUFFER_HALF_SIZE; i++) dac_buffer[i] = 0;
    }
    else
    {
        for (i = BUFFER_HALF_SIZE; i < BUFFER_FULL_SIZE; i++) dac_buffer[i] = 0;
    }
}

// 计算音频每秒字节数
static uint32_t AudioBytesPerSecond(uint32_t sampleRateHz)
{
    if (sampleRateHz == 0u)
    {
        sampleRateHz = AUDIO_DEFAULT_SAMPLE_RATE_HZ;
    }

    return sampleRateHz * AUDIO_SAMPLE_BYTES;
}

// 字节数转换为秒数
static uint32_t AudioBytesToSeconds(uint32_t bytes, uint32_t sampleRateHz)
{
    return bytes / AudioBytesPerSecond(sampleRateHz);
}

// 计算 TIMER_ADC 装载值（时钟源 BUSCLK = 80MHz）
static uint32_t CalcTimerADCLoadValue(uint32_t sampleRateHz)
{
    return (80000000u / sampleRateHz) - 1u;
}

// 计算 TIMER_DAC 装载值（时钟源 BUSCLK/2 = 40MHz）
static uint32_t CalcTimerDACLoadValue(uint32_t sampleRateHz)
{
    uint32_t dacOutputRate = sampleRateHz * INTERP_FACTOR;
    return (40000000u / dacOutputRate) - 1u;
}

// 获取当前录音采样率
static uint32_t GetCurrentRecordingSampleRateHz(void)
{
    return g_current_rec_sample_rate_hz;
}

// 计算 Bypass 模式延迟（ms）
static uint32_t CalcBypassLatencyMs(uint32_t sampleRateHz)
{
    if (sampleRateHz == 0u)
    {
        return 0u;
    }

    return (((uint32_t) BUFFER_HALF_SIZE * 1000u) + (sampleRateHz / 2u)) /
           sampleRateHz;
}

// 获取已保存的录音采样率（无效则返回默认值）
static uint32_t GetSavedRecordingSampleRateHz(void)
{
    if (saved_recording_sample_rate_hz == 0u)
    {
        return AUDIO_DEFAULT_SAMPLE_RATE_HZ;
    }

    return saved_recording_sample_rate_hz;
}

// 重置已保存的录音状态
static void ResetSavedRecordingState(void)
{
    current_flash_addr = 0u;
    playback_flash_addr = 0u;
    saved_recording_sample_rate_hz = AUDIO_DEFAULT_SAMPLE_RATE_HZ;
}

// 持久化录音元数据到 EEPROM
static bool PersistRecordingMetadata(void)
{
    uint32_t sampleRateHz = GetCurrentRecordingSampleRateHz();

    saved_recording_sample_rate_hz = sampleRateHz;
    if (current_flash_addr == 0u)
    {
        return EEPROMMetadata_Clear();
    }

    return EEPROMMetadata_SaveRecording(
        current_flash_addr, sampleRateHz, is_filter_on,
        s_global_adc_min, s_global_adc_max);
}

/*============================ UI 绘制函数 ============================*/

// 绘制水平进度条
static void DrawProgressBar(int16_t x, int16_t y, uint8_t width,
                            uint8_t height, uint8_t percent)
{
    uint8_t fill_width;

    if (percent > 100) percent = 100;

    OLED_ClearArea(x, y, width, height);

    // 用 DrawLine 画外框，避免 DrawRectangle 边框瑕疵
    OLED_DrawLine(x, y, x + width - 1, y);                                    // 上边
    OLED_DrawLine(x, y + height - 1, x + width - 1, y + height - 1);         // 下边
    OLED_DrawLine(x, y, x, y + height - 1);                                   // 左边
    OLED_DrawLine(x + width - 1, y, x + width - 1, y + height - 1);          // 右边

    fill_width = (uint16_t)percent * (width - 2) / 100;
    if (fill_width > 0)
    {
        OLED_DrawRectangle(x + 1, y + 1, fill_width, height - 2, OLED_FILLED);
    }
}

// 更新录音实时显示界面
static void UpdateRecordDisplay(uint32_t elapsed_sec, uint32_t total_sec,
                                uint32_t data_kb)
{
    uint8_t percent;

    // 第一行: 秒数
    OLED_ShowString(0, 0, "REC ", OLED_8X16);
    OLED_ShowNum(32, 0, elapsed_sec, 3, OLED_8X16);
    OLED_ShowString(56, 0, "/", OLED_8X16);
    OLED_ShowNum(64, 0, total_sec, 3, OLED_8X16);
    OLED_ShowString(88, 0, "s", OLED_8X16);

    // 第二行: 进度条 + 百分比
    percent = (total_sec > 0) ? (uint8_t)(elapsed_sec * 100 / total_sec) : 0;
    DrawProgressBar(2, 20, 84, 8, percent);
    OLED_ShowNum(90, 20, percent, 3, OLED_6X8);
    OLED_ShowString(108, 20, "%", OLED_6X8);

    // 第三行: 滤波状态
    OLED_ShowString(0, 36, "Filter:", OLED_6X8);
    if (is_filter_on)
        OLED_ShowString(48, 36, "ON ", OLED_6X8);
    else
        OLED_ShowString(48, 36, "OFF", OLED_6X8);

    // 第四行: 数据量
    OLED_ClearArea(0, 48, 136, 8);
    OLED_ShowNum(0, 48, data_kb, 4, OLED_6X8);
    OLED_ShowString(24, 48, " KB saved", OLED_6X8);

    OLED_Update();
}

// 更新播放实时显示界面
static void UpdatePlayDisplay(uint32_t elapsed_sec, uint32_t total_sec,
                              uint32_t data_kb, uint8_t percent)
{
    // 第一行: 标题 + 速度标识 + 秒数
    if (g_playback_mode == PLAY_MODE_REVERSE)
    {
        OLED_ShowString(0, 0,  "Reverse", OLED_8X16);
        OLED_ShowNum(64, 0, elapsed_sec, 3, OLED_8X16);
        OLED_ShowString(88, 0, "/", OLED_8X16);
        OLED_ShowNum(96, 0, total_sec, 3, OLED_8X16);
        OLED_ShowString(120, 0, "s", OLED_8X16);
    }
    else if (g_playspeed_mode == 2)
    {
        OLED_ShowString(0, 0, "PLAY", OLED_8X16);
        OLED_ShowString(40, 0, "2x", OLED_8X16);
        OLED_ShowNum(60, 0, elapsed_sec, 3, OLED_8X16);
        OLED_ShowString(84, 0, "/", OLED_8X16);
        OLED_ShowNum(92, 0, total_sec, 3, OLED_8X16);
        OLED_ShowString(116, 0, "s", OLED_8X16);
    }
    else
    {
        OLED_ShowString(0, 0, "PLAY ", OLED_8X16);
        OLED_ShowNum(40, 0, elapsed_sec, 3, OLED_8X16);
        OLED_ShowString(64, 0, "/", OLED_8X16);
        OLED_ShowNum(72, 0, total_sec, 3, OLED_8X16);
        OLED_ShowString(96, 0, "s", OLED_8X16);
    }

    // 第二行: 进度条 + 百分比（由调用方传入，精确到字节）
    if (percent > 100) percent = 100;
    DrawProgressBar(2, 20, 84, 8, percent);
    OLED_ShowNum(90, 20, percent, 3, OLED_6X8);
    OLED_ShowString(108, 20, "%", OLED_6X8);

    // 第三行: 数据量
    OLED_ClearArea(0, 36, 136, 8);
    OLED_ShowNum(0, 36, data_kb, 4, OLED_6X8);
    OLED_ShowString(24, 36, " KB played", OLED_6X8);

    OLED_Update();
}

// 更新 Bypass 模式显示界面
static void UpdateBypassDisplay(bool streaming)
{
    uint32_t sampleRateHz = GetCurrentRecordingSampleRateHz();
    uint32_t sampleRateKHz = (sampleRateHz + 500u) / 1000u;

    OLED_Clear();
    OLED_ShowString(0, 0, "Bypass ADC->DAC", OLED_8X16);
    OLED_ShowString(0, 16, streaming ? "Streaming" : "Warmup", OLED_8X16);
    OLED_ShowNum(80, 16, sampleRateKHz, 2, OLED_8X16);
    OLED_ShowString(96, 16, "k", OLED_8X16);
    OLED_ShowString(0, 36, "Latency:", OLED_6X8);
    OLED_ShowNum(42, 36, CalcBypassLatencyMs(sampleRateHz), 3, OLED_6X8);
    OLED_ShowString(60, 36, "ms", OLED_6X8);
    OLED_ShowString(0, 48, "2:Stop", OLED_6X8);
    OLED_Update();
}

// 将 ADC 半缓冲经滤波（可选）后写入 Flash
void FlushFlashBuffer(volatile uint16_t *src, uint16_t half_offset)
{
    if (is_filter_on)
    {
        uint16_t offset = 0;

        while (offset < BUFFER_HALF_SIZE)
        {
            uint16_t chunk_samples = BUFFER_HALF_SIZE - offset;

            if (chunk_samples > FILTER_WRITE_CHUNK_SAMPLES)
            {
                chunk_samples = FILTER_WRITE_CHUNK_SAMPLES;
            }

            AudioFilter_Process(&src[half_offset + offset],
                                s_filter_write_buffer,
                                chunk_samples);
            {
                uint16_t k;
                for (k = 0; k < chunk_samples; k++) {
                    uint16_t s = s_filter_write_buffer[k];
                    if (s < s_global_adc_min) s_global_adc_min = s;
                    if (s > s_global_adc_max) s_global_adc_max = s;
                }
            }
            W25Q64_BufferWrite(current_flash_addr + ((uint32_t)offset * sizeof(uint16_t)),
                (uint8_t *)s_filter_write_buffer,
                (uint32_t)chunk_samples * sizeof(uint16_t));
            offset += chunk_samples;
        }
    }
    else
    {
        uint16_t k;
        for (k = 0; k < BUFFER_HALF_SIZE; k++) {
            uint16_t s = src[half_offset + k];
            if (s < s_global_adc_min) s_global_adc_min = s;
            if (s > s_global_adc_max) s_global_adc_max = s;
        }
        W25Q64_BufferWrite(current_flash_addr,
            (uint8_t *)&src[half_offset], BUFFER_HALF_SIZE * 2);
    }
    current_flash_addr += BUFFER_HALF_SIZE * 2;
}

// 状态切换：退出旧状态 → 更新状态变量 → 进入新状态
void State_Transition(uint8_t new_state)
{
    if (new_state >= STATE_COUNT) return;
    if (new_state == g_current_state) return;

    // 调用旧状态 on_exit
    if (g_current_state < STATE_COUNT)
    {
        if (state_table[g_current_state].on_exit != NULL)
            state_table[g_current_state].on_exit();
    }

    g_current_state = new_state;

    // 调用新状态 on_enter
    if (state_table[g_current_state].on_enter != NULL)
        state_table[g_current_state].on_enter();
}

/*============================ STATE_IDLE 主菜单 ============================*/

static void Idle_OnEnter(void)
{
    uint32_t savedSampleRateKHz;

    OLED_Clear();
    OLED_ShowString(10, 0,  "== Audio Replay ==", OLED_6X8);
    OLED_ShowString(10, 8,  "1:Record  2:Play", OLED_6X8);
    OLED_ShowString(10, 16, "3:2xSpd   4:UART", OLED_6X8);
    OLED_ShowString(10, 24, "5:Reverse 6:FrqTest", OLED_6X8);
    OLED_ShowString(10, 32, "7:Erase   8:Bypass", OLED_6X8);
    if (current_flash_addr > 0u)
    {
        savedSampleRateKHz = (GetSavedRecordingSampleRateHz() + 500u) / 1000u;
        OLED_ShowString(0, 56, "Saved", OLED_6X8);
        OLED_ShowNum(32, 56,
            AudioBytesToSeconds(current_flash_addr, GetSavedRecordingSampleRateHz()),
            3, OLED_6X8);
        OLED_ShowString(50, 56, "s", OLED_6X8);
        OLED_ShowNum(62, 56, current_flash_addr / 1024, 4, OLED_6X8);
        OLED_ShowString(86, 56, "K", OLED_6X8);
        OLED_ShowNum(98, 56, savedSampleRateKHz, 2, OLED_6X8);
        OLED_ShowString(110, 56, "k", OLED_6X8);
    }
    else
    {
        OLED_ShowString(0, 56, "No saved audio", OLED_6X8);
    }
    OLED_Update();
}

static void Idle_OnExit(void)
{
    // 无需清理
}

static void Idle_HandleKey(uint8_t key)
{
    uint32_t erase_addr;
    bool success;

    switch (key)
    {
    // 按1: 录音
    case 1:
        if (g_current_state != STATE_IDLE) break;

        ResetSavedRecordingState();

        OLED_Clear();
        OLED_ShowString(0, 0, "Erasing Flash...", OLED_8X16);
        OLED_Update();

        for (erase_addr = 0; erase_addr < RECORD_MAX_BYTES; erase_addr += BLOCK_64KB_SIZE)
        {
            W25Q64_BlockErase_64KB(erase_addr);
            OLED_ShowNum(0, 16, (erase_addr * 100) / RECORD_MAX_BYTES, 3, OLED_8X16);
            OLED_ShowString(32, 16, "%", OLED_8X16);
            OLED_Update();
        }

        {
            uint8_t rate_key;
            OLED_Clear();
            OLED_ShowString(0, 0,  "Erase Done!", OLED_8X16);
            OLED_ShowString(0, 16, "Sample Rate?", OLED_8X16);
            OLED_ShowString(0, 32, "1:16kHz  2:32kHz", OLED_6X8);
            OLED_Update();

            do {
                rate_key = GetKeyNum_NonBlocking();
            } while (rate_key != 1 && rate_key != 2);

            g_current_rec_sample_rate_hz = (rate_key == 2) ? 32000u : 16000u;
            delay_ms(200);
        }

        {
            if (g_current_rec_sample_rate_hz == 16000u)
            {
                uint8_t filter_key;
                OLED_Clear();
                OLED_ShowString(0, 0,  "16kHz Mode", OLED_8X16);
                OLED_ShowString(0, 16, "is filter on?", OLED_8X16);
                OLED_ShowString(0, 32, "1:ON   2:OFF", OLED_6X8);
                OLED_Update();

                do {
                    filter_key = GetKeyNum_NonBlocking();
                } while (filter_key != 1 && filter_key != 2);

                is_filter_on = (filter_key == 1);
            }
            else
            {
                is_filter_on = false;
            }
            delay_ms(200);
        }

        if (!EEPROMMetadata_Clear())
        {
            OLED_Clear();
            OLED_ShowString(0, 0, "Meta Clear Fail", OLED_8X16);
            OLED_Update();
            WaitForKey();
            Idle_OnEnter();
            break;
        }

        State_Transition(STATE_RECORDING);
        break;

    // 按2: 正常播放（1x 速）
    case 2:
        if (current_flash_addr > 0)
        {
            g_playspeed_mode = 1;
            g_playback_mode = PLAY_MODE_NORMAL;
            State_Transition(STATE_PLAYING);
        }
        break;

    // 按3: 2倍速播放
    case 3:
        if (current_flash_addr > 0)
        {
            g_playspeed_mode = 2;
            g_playback_mode = PLAY_MODE_NORMAL;
            State_Transition(STATE_PLAYING);
        }
        break;

    // 按4: UART 发送音频数据
    case 4:
        if (current_flash_addr > 0)
        {
            SendAudioVoltageToVofaPlus();

            OLED_Clear();
            OLED_ShowString(0, 0, "UART Send Done!", OLED_8X16);
            OLED_Update();
            WaitForKey();
            Idle_OnEnter();
        }
        break;

    // 按5: 倒放（1x 速）
    case 5:
        if (current_flash_addr > 0)
        {
            g_playspeed_mode = 1;
            g_playback_mode = PLAY_MODE_REVERSE;
            State_Transition(STATE_PLAYING);
        }
        break;

    // 按6: 频率/信噪比测试
    case 6:
        State_Transition(STATE_FREQ_TEST);
        break;

    // 按7: 擦除子菜单
    case 7:
    {
        uint8_t erase_key;

        OLED_Clear();
        OLED_ShowString(0, 0,  "Erase Menu", OLED_8X16);
        OLED_ShowString(0, 16, "1:W25Q64", OLED_6X8);
        OLED_ShowString(0, 24, "2:EEPROM", OLED_6X8);
        OLED_ShowString(0, 32, "3:Exit", OLED_6X8);
        OLED_Update();

        do {
            erase_key = GetKeyNum_NonBlocking();
        } while (erase_key != 1 && erase_key != 2 && erase_key != 3);
        delay_ms(200);

        if (erase_key == 3)
        {
            Idle_OnEnter();
            break;
        }

        if (erase_key == 1)
        {
            OLED_Clear();
            OLED_ShowString(0, 0, "Erasing W25Q64", OLED_8X16);
            OLED_ShowString(0, 16, "Please Wait...", OLED_8X16);
            OLED_Update();

            W25Q64_ChipErase();
            ResetSavedRecordingState();
            success = EEPROMMetadata_Clear();

            OLED_Clear();
            OLED_ShowString(0, 0, "W25Q64 Erased", OLED_8X16);
            OLED_ShowString(0, 16,
                success ? "Meta Cleared" : "Meta Clear Fail", OLED_8X16);
            OLED_Update();
        }
        else
        {
            OLED_Clear();
            OLED_ShowString(0, 0, "Erasing EEPROM", OLED_8X16);
            OLED_ShowString(0, 16, "Data Area...", OLED_8X16);
            OLED_Update();

            success = EEPROMMetadata_EraseAll();
            if (success)
            {
                ResetSavedRecordingState();
            }

            OLED_Clear();
            OLED_ShowString(0, 0,
                success ? "EEPROM Erased" : "EEPROM Erase NG", OLED_8X16);
            OLED_ShowString(0, 16,
                success ? "0x41D0_0000 OK" : "Data Area Failed", OLED_8X16);
            OLED_Update();
        }
        WaitForKey();
        Idle_OnEnter();
        break;
    }

    // 按8: Bypass 直录直放
    case 8:
    {
        uint8_t direct_key;

        OLED_Clear();
        OLED_ShowString(0, 0, "Bypass Mode", OLED_8X16);
        OLED_ShowString(0, 16, "Sample Rate?", OLED_8X16);
        OLED_ShowString(0, 32, "1:16kHz  2:32kHz", OLED_6X8);
        OLED_Update();

        do {
            direct_key = GetKeyNum_NonBlocking();
        } while (direct_key != 1 && direct_key != 2);

        g_current_rec_sample_rate_hz = (direct_key == 1) ? 16000u : 32000u;
        delay_ms(200);
        State_Transition(STATE_BYPASS);
        break;
    }

    default:
        break;
    }
}

static void Idle_BackgroundTask(void)
{
    // 无后台任务
}

/*============================ STATE_RECORDING 录音态 ============================*/

static void Recording_OnEnter(void)
{
    flash_write_flag = 0;
    s_global_adc_min = 4095;
    s_global_adc_max = 0;

    // 设置 TIMER_ADC 采样频率
    DL_TimerA_setLoadValue(TIMER_ADC_INST, CalcTimerADCLoadValue(g_current_rec_sample_rate_hz));

    // 配置 DMA CH0: ADC → adc_buffer
    DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID,
        DL_ADC12_getMemResultAddress(ADC12_0_INST, DL_ADC12_MEM_IDX_0));
    DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&adc_buffer[0]);
    DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, BUFFER_FULL_SIZE);
    DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);

    DL_ADC12_enableConversions(ADC12_0_INST);
    DL_TimerA_startCounter(TIMER_ADC_INST);

    // 初始化录音界面
    s_rec_last_sec = 0xFFFFFFFF;
    OLED_Clear();
    UpdateRecordDisplay(
        0, AudioBytesToSeconds(RECORD_MAX_BYTES, GetCurrentRecordingSampleRateHz()), 0);
}

static void Recording_OnExit(void)
{
    bool metadataSaved;

    DL_TimerA_stopCounter(TIMER_ADC_INST);
    DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);

    // 刷写缓冲区残留数据
    if ((flash_write_flag == 1) && (current_flash_addr < RECORD_MAX_BYTES))
    {
        FlushFlashBuffer(adc_buffer, 0);
    }
    else if ((flash_write_flag == 2) && (current_flash_addr < RECORD_MAX_BYTES))
    {
        FlushFlashBuffer(adc_buffer, BUFFER_HALF_SIZE);
    }

    flash_write_flag = 0;
    metadataSaved = PersistRecordingMetadata();

    // 显示录音结果
    OLED_Clear();
    OLED_ShowString(0, 0,
        metadataSaved ? "Record Stopped!" : "Meta Save Fail", OLED_8X16);
    OLED_ShowNum(0, 16, current_flash_addr / 1024, 4, OLED_8X16);
    OLED_ShowString(40, 16, "KB Saved", OLED_8X16);
    OLED_Update();
    WaitForKey();
}

static void Recording_HandleKey(uint8_t key)
{
    if (key == 2)
    {
        // 停止录音，回到主菜单
        State_Transition(STATE_IDLE);
    }
}

static void Recording_BackgroundTask(void)
{
    // DMA 半完成中断触发，将数据写入 Flash
    if (flash_write_flag == 1)
    {
        FlushFlashBuffer(adc_buffer, 0);
        flash_write_flag = 0;
    }

    if (flash_write_flag == 2)
    {
        FlushFlashBuffer(adc_buffer, BUFFER_HALF_SIZE);
        flash_write_flag = 0;
    }

    // 秒数变化时刷新显示
    {
        uint32_t currentSampleRateHz = GetCurrentRecordingSampleRateHz();
        uint32_t rec_sec = AudioBytesToSeconds(
            current_flash_addr, currentSampleRateHz);
        if (rec_sec != s_rec_last_sec)
        {
            s_rec_last_sec = rec_sec;
            UpdateRecordDisplay(rec_sec,
                AudioBytesToSeconds(RECORD_MAX_BYTES, currentSampleRateHz),
                current_flash_addr / 1024);
        }
    }

    // 录满自动停止
    if (current_flash_addr >= RECORD_MAX_BYTES)
    {
        bool metadataSaved;

        DL_TimerA_stopCounter(TIMER_ADC_INST);
        DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);

        flash_write_flag = 0;
        metadataSaved = PersistRecordingMetadata();

        OLED_Clear();
        OLED_ShowString(0, 0,
            metadataSaved ? "Record Done!" : "Meta Save Fail", OLED_8X16);
        OLED_ShowString(0, 16,
            metadataSaved ? "Memory Full" : "Memory Saved", OLED_8X16);
        OLED_ShowNum(0, 32, current_flash_addr / 1024, 4, OLED_6X8);
        OLED_ShowString(24, 32, " KB", OLED_6X8);
        OLED_Update();

        WaitForKey();

        // 直接回到主菜单（跳过 on_exit）
        g_current_state = STATE_IDLE;
        Idle_OnEnter();
    }
}

/*============================ STATE_PLAYING 播放态 ============================*/
// 计块法：录制时统计总块数，播放时严格播完同等块数后停止

static void Playing_OnEnter(void)
{
    if (current_flash_addr == 0u)
    {
        g_current_state = STATE_IDLE;
        Idle_OnEnter();
        return;
    }

    // 计块法初始化（每 raw 块 = 1024 样本, 插值后填满一个 DMA 半缓冲）
    s_total_play_blocks = current_flash_addr / (INTERP_CHUNK_RAW_SIZE * sizeof(uint16_t));
    s_blocks_played     = 0;
    s_blocks_fed        = 0;

    /* 清零缓冲区 */
    {
        uint16_t i;
        for (i = 0; i < BUFFER_FULL_SIZE; i++) dac_buffer[i] = 0;
    }

    if (g_playback_mode == PLAY_MODE_REVERSE)
    {
        /* 倒放：从末尾向前逐 raw 块读 + 插值 + 反转 */
        OLED_Clear();
        OLED_ShowString(0, 0, "REV Pre-fill...", OLED_8X16);
        OLED_Update();

        playback_flash_addr = current_flash_addr - INTERP_CHUNK_RAW_SIZE * sizeof(uint16_t);

        /* 前半缓冲 = 最后一块 raw (最先被 DMA 消费) */
        FillInterpolatedHalf(playback_flash_addr, 0, true);
        s_blocks_fed++;

        /* 后半缓冲 = 倒数第二块 raw */
        if (s_blocks_fed < s_total_play_blocks) {
            playback_flash_addr -= INTERP_CHUNK_RAW_SIZE * sizeof(uint16_t);
            FillInterpolatedHalf(playback_flash_addr, 1, true);
            s_blocks_fed++;
        }
    }
    else
    {
        /* 正常播放：从头部顺序逐 raw 块读 + 插值 */
        OLED_Clear();
        OLED_ShowString(0, 0, "Pre-filling...", OLED_8X16);
        OLED_Update();

        playback_flash_addr = 0;

        /* 前半缓冲 = 第 0 块 raw */
        FillInterpolatedHalf(playback_flash_addr, 0, false);
        playback_flash_addr += INTERP_CHUNK_RAW_SIZE * sizeof(uint16_t);
        s_blocks_fed++;

        /* 后半缓冲 = 第 1 块 raw */
        if (s_blocks_fed < s_total_play_blocks) {
            FillInterpolatedHalf(playback_flash_addr, 1, false);
            playback_flash_addr += INTERP_CHUNK_RAW_SIZE * sizeof(uint16_t);
            s_blocks_fed++;
        }
    }

    // 设置 TIMER_DAC 为录音采样率 ÷ 速度倍率
    DL_TimerG_setLoadValue(TIMER_DAC_INST,
        CalcTimerDACLoadValue(GetSavedRecordingSampleRateHz()) / g_playspeed_mode);

    // 配置 DMA CH1: dac_buffer → DAC
    dac_read_flag = 0;
    DL_DMA_setSrcAddr(DMA, DMA_CH1_CHAN_ID, (uint32_t)&dac_buffer[0]);
    DL_DMA_setDestAddr(DMA, DMA_CH1_CHAN_ID, (uint32_t)&DAC0->DATA0);
    DL_DMA_setTransferSize(DMA, DMA_CH1_CHAN_ID, BUFFER_FULL_SIZE);
    DL_DMA_enableChannel(DMA, DMA_CH1_CHAN_ID);

    DL_DAC12_enableOutputPin(DAC0);
    DL_TimerG_startCounter(TIMER_DAC_INST);

    // 初始化播放界面
    {
        uint32_t total_sec = AudioBytesToSeconds(
            current_flash_addr, GetSavedRecordingSampleRateHz());
        s_play_last_sec = 0xFFFFFFFF;
        OLED_Clear();
        UpdatePlayDisplay(0, total_sec, 0, 0);
    }
}

static void Playing_OnExit(void)
{
    DL_TimerG_stopCounter(TIMER_DAC_INST);
    DL_DMA_disableChannel(DMA, DMA_CH1_CHAN_ID);
    DL_DAC12_output12(DAC0, 0);

    dac_read_flag = 0;
    g_playspeed_mode = 1;  // 恢复正常速度

    // 显示播放结果
    OLED_Clear();
    OLED_ShowString(0, 0, "Play Stopped!", OLED_8X16);
    OLED_ShowNum(0, 16, s_blocks_played * (INTERP_CHUNK_RAW_SIZE * 2u) / 1024, 4, OLED_8X16);
    OLED_ShowString(40, 16, "KB Played", OLED_8X16);
    OLED_Update();
    WaitForKey();
}

static void Playing_HandleKey(uint8_t key)
{
    if (key == 2)
    {
        // 停止播放，回到主菜单
        State_Transition(STATE_IDLE);
    }
}

// 播放后台任务 — 计块法核心
// 流程：DMA 播完一块 → blocks_played++ → 若未播完则从 Flash 续读下一块
static void Playing_BackgroundTask(void)
{
    uint8_t flag;

    // 取出 DMA 中断标志
    flag = dac_read_flag;
    if (flag == 0)
    {
        // 无新中断，仅节流刷新显示
        uint32_t savedSampleRateHz = GetSavedRecordingSampleRateHz();
        uint32_t played_bytes = s_blocks_played * (INTERP_CHUNK_RAW_SIZE * sizeof(uint16_t));
        uint32_t total_sec = AudioBytesToSeconds(
            current_flash_addr, savedSampleRateHz);
        uint32_t play_sec  = AudioBytesToSeconds(
            played_bytes, savedSampleRateHz);
        if (play_sec != s_play_last_sec)
        {
            uint8_t percent = (current_flash_addr > 0u)
                ? (uint8_t)(played_bytes * 100u / current_flash_addr) : 0;
            s_play_last_sec = play_sec;
            UpdatePlayDisplay(play_sec, total_sec, played_bytes / 1024, percent);
        }
        return;
    }

    dac_read_flag = 0;

    // 步骤1: 已播完一个半缓冲
    s_blocks_played++;

    // 步骤2: 检查是否全部播完
    if (s_blocks_played >= s_total_play_blocks)
    {
        DL_TimerG_stopCounter(TIMER_DAC_INST);
        DL_DMA_disableChannel(DMA, DMA_CH1_CHAN_ID);
        DL_DAC12_output12(DAC0, 0);

        g_playspeed_mode = 1;  // 恢复正常速度

        OLED_Clear();
        if (g_playback_mode == PLAY_MODE_REVERSE)
            OLED_ShowString(0, 0, "REV Done!", OLED_8X16);
        else
        {
            OLED_ShowString(0, 0, "Play Done!", OLED_8X16);
            OLED_ShowNum(0, 16, s_blocks_played * (INTERP_CHUNK_RAW_SIZE * 2u) / 1024,
                         4, OLED_6X8);
            OLED_ShowString(24, 16, " KB played", OLED_6X8);
        }
        OLED_Update();
        WaitForKey();

        g_current_state = STATE_IDLE;
        Idle_OnEnter();
        return;
    }

    // 步骤3: 从 Flash 读 raw 块 → 插值 → 填入刚消费完的半缓冲
    if (s_blocks_fed < s_total_play_blocks)
    {
        if (g_playback_mode == PLAY_MODE_REVERSE)
        {
            playback_flash_addr -= INTERP_CHUNK_RAW_SIZE * sizeof(uint16_t);
            FillInterpolatedHalf(playback_flash_addr, (uint8_t)(flag - 1), true);
        }
        else
        {
            FillInterpolatedHalf(playback_flash_addr, (uint8_t)(flag - 1), false);
            playback_flash_addr += INTERP_CHUNK_RAW_SIZE * sizeof(uint16_t);
        }
        s_blocks_fed++;
    }
    else
    {
        ClearHalfBuffer(flag - 1);
    }
}

/*============================ STATE_FREQ_TEST 频率/信噪比测试态 ============================*/

static FreqSNR_Result_t s_freq_result;              // 测试结果
static uint8_t s_freq_measured = 0;                  // 测量完成标志

static void FreqTest_OnEnter(void)
{
    fft_data_ready = 0;
    s_freq_measured = 0;
    g_adc_mode = 1;  // 切换到频率测试模式

    // 固定 16kHz 采样率
    DL_TimerA_setLoadValue(TIMER_ADC_INST, CalcTimerADCLoadValue(16000u));

    // 配置 DMA CH0: ADC → adc_buffer
    DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID,
        DL_ADC12_getMemResultAddress(ADC12_0_INST, DL_ADC12_MEM_IDX_0));
    DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&adc_buffer[0]);
    DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, BUFFER_FULL_SIZE);
    DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);

    DL_ADC12_enableConversions(ADC12_0_INST);
    DL_TimerA_startCounter(TIMER_ADC_INST);

    // 显示等待采样界面
    OLED_Clear();
    OLED_ShowString(0, 0,  "Freq & SNR Test", OLED_8X16);
    OLED_ShowString(0, 16, "Sampling...", OLED_8X16);
    OLED_ShowString(0, 32, "2:Stop", OLED_6X8);
    OLED_Update();
}

static void FreqTest_OnExit(void)
{
    DL_TimerA_stopCounter(TIMER_ADC_INST);
    DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);

    fft_data_ready = 0;
    g_adc_mode = 0;  // 恢复录音模式
}

static void FreqTest_HandleKey(uint8_t key)
{
    if (key == 2)
    {
        State_Transition(STATE_IDLE);
    }
}

static void FreqTest_BackgroundTask(void)
{
    if (fft_data_ready != 0)
    {
        uint16_t half_offset;

        // 确定有效数据的半缓冲偏移
        if (fft_data_ready == 1)
            half_offset = 0;
        else
            half_offset = BUFFER_HALF_SIZE;

        fft_data_ready = 0;

        // 暂停 DMA，防止数据覆盖
        DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);

        // 执行 FFT 频率 + SNR 分析
        FreqSNR_Process(&adc_buffer[half_offset], BUFFER_HALF_SIZE, &s_freq_result);
        s_freq_measured = 1;

        // 重启 DMA 继续采样
        DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID,
            DL_ADC12_getMemResultAddress(ADC12_0_INST, DL_ADC12_MEM_IDX_0));
        DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&adc_buffer[0]);
        DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, BUFFER_FULL_SIZE);
        DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);

        // 更新测试结果界面
        OLED_Clear();
        if (s_freq_result.valid)
        {
            OLED_ShowString(0, 0,  "Freq Test Result", OLED_6X8);

            // 频率
            OLED_ShowString(0, 10, "Freq:", OLED_6X8);
            OLED_ShowNum(30, 10, (uint32_t)s_freq_result.frequency_hz, 5, OLED_6X8);
            OLED_ShowString(62, 10, "Hz", OLED_6X8);

            // SNR
            OLED_ShowString(0, 22, "SNR:", OLED_6X8);
            if (s_freq_result.snr_db >= 0)
            {
                OLED_ShowString(28, 22, "+", OLED_6X8);
                OLED_ShowNum(34, 22, (uint32_t)s_freq_result.snr_db, 3, OLED_6X8);
            }
            else
            {
                OLED_ShowString(28, 22, "-", OLED_6X8);
                OLED_ShowNum(34, 22, (uint32_t)(-s_freq_result.snr_db), 3, OLED_6X8);
            }
            OLED_ShowString(54, 22, "dB", OLED_6X8);

            // 峰值 bin
            OLED_ShowString(0, 34, "Bin:", OLED_6X8);
            OLED_ShowNum(26, 34, s_freq_result.peak_bin, 4, OLED_6X8);

            // 幅度
            OLED_ShowString(0, 46, "Mag:", OLED_6X8);
            OLED_ShowNum(26, 46, (uint32_t)(s_freq_result.peak_magnitude * 10000), 5, OLED_6X8);
        }
        else
        {
            OLED_ShowString(0, 0,  "Freq Test Result", OLED_6X8);
            OLED_ShowString(0, 16, "No signal!", OLED_8X16);
        }

        OLED_ShowString(80, 54, "2:Stop", OLED_6X8);
        OLED_Update();
    }
}

/*============================ STATE_BYPASS 直录直放态 ============================*/

static void ByPass_OnEnter(void)
{
    uint32_t i;

    s_bypass_running = true;
    s_bypass_dac_started = false;
    flash_write_flag = 0;
    dac_read_flag = 0;

    for (i = 0u; i < BUFFER_FULL_SIZE; i++)
    {
        adc_buffer[i] = 0u;
    }

    DL_TimerA_setLoadValue(TIMER_ADC_INST,
        CalcTimerADCLoadValue(g_current_rec_sample_rate_hz));
    DL_TimerG_setLoadValue(TIMER_DAC_INST,
        CalcTimerDACLoadValue(g_current_rec_sample_rate_hz));

    DL_TimerG_stopCounter(TIMER_DAC_INST);
    DL_DMA_disableChannel(DMA, DMA_CH1_CHAN_ID);
    DL_DAC12_output12(DAC0, 0);

    DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID,
        DL_ADC12_getMemResultAddress(ADC12_0_INST, DL_ADC12_MEM_IDX_0));
    DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&adc_buffer[0]);
    DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, BUFFER_FULL_SIZE);
    DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);

    DL_ADC12_enableConversions(ADC12_0_INST);
    DL_TimerA_startCounter(TIMER_ADC_INST);

    UpdateBypassDisplay(false);
}

static void ByPass_OnExit(void)
{
    uint32_t i;

    s_bypass_running = false;
    s_bypass_dac_started = false;

    DL_TimerA_stopCounter(TIMER_ADC_INST);
    DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
    DL_TimerG_stopCounter(TIMER_DAC_INST);
    DL_DMA_disableChannel(DMA, DMA_CH1_CHAN_ID);
    DL_DAC12_output12(DAC0, 0);
    flash_write_flag = 0;
    dac_read_flag = 0;

    for (i = 0u; i < BUFFER_FULL_SIZE; i++)
    {
        adc_buffer[i] = 0u;
    }

    OLED_Clear();
    OLED_ShowString(0, 0, "Bypass Stopped!", OLED_8X16);
    OLED_Update();
    WaitForKey();
}

static void ByPass_HandleKey(uint8_t key)
{
    if (key == 2)
    {
        State_Transition(STATE_IDLE);
    }
}

static void ByPass_BackgroundTask(void)
{
    if (!s_bypass_running)
    {
        return;
    }

    // 等待 ADC 首次半完成中断后启动 DAC
    if (!s_bypass_dac_started)
    {
        if (flash_write_flag == 1u)
        {
            flash_write_flag = 0;
            dac_read_flag = 0;

            DL_DMA_setSrcAddr(DMA, DMA_CH1_CHAN_ID, (uint32_t)&dac_buffer[0]);
            DL_DMA_setDestAddr(DMA, DMA_CH1_CHAN_ID, (uint32_t)&DAC0->DATA0);
            DL_DMA_setTransferSize(DMA, DMA_CH1_CHAN_ID, BUFFER_FULL_SIZE);
            DL_DMA_enableChannel(DMA, DMA_CH1_CHAN_ID);

            DL_DAC12_enableOutputPin(DAC0);
            DL_TimerG_startCounter(TIMER_DAC_INST);

            s_bypass_dac_started = true;
            UpdateBypassDisplay(true);
        }
        return;
    }

    // 持续运行中，清除中断标志
    if (flash_write_flag != 0u)
    {
        flash_write_flag = 0;
    }

    if (dac_read_flag != 0u)
    {
        dac_read_flag = 0;
    }
}

/*============================ 状态表 ============================*/
AppState state_table[STATE_COUNT] = {
    /* STATE_IDLE       */ { Idle_OnEnter,       Idle_OnExit,       Idle_HandleKey,       Idle_BackgroundTask       },
    /* STATE_RECORDING  */ { Recording_OnEnter,  Recording_OnExit,  Recording_HandleKey,  Recording_BackgroundTask  },
    /* STATE_PLAYING    */ { Playing_OnEnter,    Playing_OnExit,    Playing_HandleKey,    Playing_BackgroundTask    },
    /* STATE_FREQ_TEST  */ { FreqTest_OnEnter,   FreqTest_OnExit,   FreqTest_HandleKey,   FreqTest_BackgroundTask   },
    /* STATE_BYPASS     */ { ByPass_OnEnter,     ByPass_OnExit,     ByPass_HandleKey,     ByPass_BackgroundTask     },
};