#include "ti_msp_dl_config.h"
#include "bsp.h"
#include "MenuSys.h"
#include "eeprom_metadata.h"
#include "freq_snr_test.h"

/*============================ 全局变量 ============================*/
volatile AudioSharedBuffer_t audio_shared_buf;     // 录音/播放/FFT 共享缓冲
volatile uint8_t  flash_write_flag = 0;            // 0:空闲 1:写前半 2:写后半
volatile uint8_t  dac_read_flag = 0;               // 0:空闲 1:读前半 2:读后半

/*--- 频率测试模式相关 ---*/
volatile uint8_t  g_adc_mode = 0;        // 0=录音模式, 1=频率测试模式
volatile uint8_t  fft_data_ready = 0;    // 0=空闲 1=前半就绪 2=后半就绪

uint32_t current_flash_addr  = 0;    // 录音 Flash 写指针
uint32_t playback_flash_addr = 0;    // 播放 Flash 读指针
uint32_t saved_recording_sample_rate_hz = AUDIO_DEFAULT_SAMPLE_RATE_HZ;
bool is_filter_on = true;            // 滤波是否开启

uint8_t MID;                         // Flash 厂商 ID
uint16_t DID;                        // Flash 设备 ID

static void RestoreRecordingMetadata(void)
{
    EEPROMRecordingMetadata metadata;

    saved_recording_sample_rate_hz = AUDIO_DEFAULT_SAMPLE_RATE_HZ;
    current_flash_addr = 0u;
    if (!EEPROMMetadata_Load(&metadata)) {
        return;
    }

    if (!metadata.hasRecording) {
        return;
    }

    if ((metadata.recordingBytes == 0u) ||
        (metadata.recordingBytes > RECORD_MAX_BYTES) ||
        ((metadata.recordingBytes % (BUFFER_HALF_SIZE * 2u)) != 0u)) {
        return;
    }

    if (metadata.sampleRateHz == 0u) {
        return;
    }

    current_flash_addr = metadata.recordingBytes;
    saved_recording_sample_rate_hz = metadata.sampleRateHz;
    is_filter_on = metadata.filterEnabled;
    s_global_adc_min = metadata.globalAdcMin;
    s_global_adc_max = metadata.globalAdcMax;
}

/*============================ 主函数 ============================*/
int main(void)
{
    /*--- 初始化 ---*/
    SYSCFG_DL_init();
    OLED_Init();
    W25Q64_Init();
    AudioFilter_Init();
    FreqSNR_Init();
    RestoreRecordingMetadata();

    NVIC_EnableIRQ(ADC12_0_INST_INT_IRQN);
    NVIC_EnableIRQ(TIMER_KEY_INST_INT_IRQN);
    NVIC_EnableIRQ(DMA_INT_IRQn);

    /*--- 开机画面 ---*/
    OLED_Clear();
    OLED_ShowImage(0, 0, 128, 64, Ark);
    OLED_Update();
    delay_ms(500);

    /*--- Flash ID 验证 ---*/
    W25Q64_ReadID(&MID, &DID);
    OLED_Clear();
    OLED_ShowString(0, 0,  "MID:", OLED_8X16);
    OLED_ShowHexNum(32, 0, MID, 2, OLED_8X16);
    OLED_ShowString(0, 16, "DID:", OLED_8X16);
    OLED_ShowHexNum(32, 16, DID, 4, OLED_8X16);
    OLED_ShowString(0, 32, "SPI Flash Ready", OLED_8X16);
    OLED_Update();
    delay_ms(500);

    /*--- 进入主菜单（IDLE 状态） ---*/
    State_Transition(STATE_IDLE);

    /*--- 主循环 ---*/
    while (1)
    {
        uint8_t key = GetKeyNum_NonBlocking();

        if (key != 0)
        {
            if (state_table[g_current_state].handle_key != NULL)
                state_table[g_current_state].handle_key(key);
        }

        if (state_table[g_current_state].background_task != NULL)
            state_table[g_current_state].background_task();
    }
}

/*============================ DMA 中断 ============================*/
void DMA_IRQHandler(void)
{
    uint32_t irq = DL_DMA_getPendingInterrupt(DMA);

    // CH0: 前半满 — 根据模式路由
    if (irq == DL_DMA_FULL_CH_EVENT_IIDX_EARLY_IRQ_DMACH0)
    {
        if (g_adc_mode == 1)
            fft_data_ready = 1;
        else
            flash_write_flag = 1;
    }
    // CH0: 后半满 — 根据模式路由
    else if (irq == DL_DMA_EVENT_IIDX_DMACH0)
    {
        if (g_adc_mode == 1)
            fft_data_ready = 2;
        else
            flash_write_flag = 2;
    }
    // CH1 播放: 前半空
    else if (irq == DL_DMA_FULL_CH_EVENT_IIDX_EARLY_IRQ_DMACH1)
    {
        dac_read_flag = 1;
    }
    // CH1 播放: 后半空
    else if (irq == DL_DMA_EVENT_IIDX_DMACH1)
    {
        DL_GPIO_togglePins(LED_PORT, LED_PIN_1_PIN);
        dac_read_flag = 2;
    }
}