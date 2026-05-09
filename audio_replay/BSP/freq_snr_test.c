/**
 * @file    freq_snr_test.c
 * @brief   基于 CMSIS-DSP RFFT Q15 的频率与信噪比测量实现
 *
 * 算法流程：
 *   1. ADC 12-bit 数据 → Q15 格式转换（去 DC + 缩放）
 *   2. 加 Hann 窗减少频谱泄漏
 *   3. 2048 点 RFFT Q15
 *   4. 计算幅度谱 → 找峰值 bin → 频率
 *   5. 信号功率 / 噪声功率 → SNR (dB)
 */

#include "freq_snr_test.h"
#include "MenuSys.h"
#include "arm_math.h"
#include <math.h>

/*============================ 私有变量 ============================*/
/* RFFT 实例 */
static arm_rfft_instance_q15  s_rfft_inst;

/* 临时工作缓冲（Q15 FFT 内部使用） */
static q15_t s_work_buffer[FFT_SIZE];

/*============================ 私有函数 ============================*/

/**
 * @brief  简易 log10 近似（用于 dB 换算）
 *         使用 log2(x) * log10(2) = log2(x) * 0.30103
 *         log2 通过 frexp() 提取指数 + 尾数多项式近似
 */
static float approx_log10f(float x)
{
    if (x <= 0.0f) return -96.0f; /* 下限 */

    /* 利用 frexp 分离指数和尾数 */
    int exp2;
    float mantissa = frexpf(x, &exp2); /* mantissa ∈ [0.5, 1.0) */

    /* 尾数的多项式近似 ln(m) = log2(m)/log2(e)
       简化：ln(m) ≈ (m-1) - (m-1)^2/2 + (m-1)^3/3  (Taylor @ m=1)
       但 mantissa ∈ [0.5,1), 用 m*2 ∈ [1,2), 设 t = m*2 - 1 ∈ [0,1)
    */
    float t = mantissa * 2.0f - 1.0f;
    float ln_mantissa = t - t * t * 0.5f + t * t * t * 0.3333f;

    /* log2(x) = exp2 + log2(mantissa) = exp2 + ln_mantissa / ln(2) */
    float log2_x = (float)exp2 - 1.0f + ln_mantissa / 0.693147f;

    /* log10(x) = log2(x) * log10(2) */
    return log2_x * 0.30103f;
}

/**
 * @brief  抛物线插值获取亚 bin 频率精度
 * @param  mag_left   峰值左侧 bin 的幅度
 * @param  mag_center 峰值 bin 的幅度
 * @param  mag_right  峰值右侧 bin 的幅度
 * @return 峰值相对于 center bin 的偏移量（-0.5 ~ +0.5）
 */
static float parabolic_interp(float mag_left, float mag_center, float mag_right)
{
    float denom = 2.0f * (2.0f * mag_center - mag_left - mag_right);
    if (fabsf(denom) < 1e-10f) return 0.0f;
    return (mag_left - mag_right) / denom;
}

static q15_t hann_window_q15(uint16_t index)
{
    float w = 0.5f * (1.0f - cosf(2.0f * 3.14159265f * (float)index / (float)(FFT_SIZE - 1)));
    return (q15_t)(w * 32767.0f);
}

static q15_t *fft_output_buffer(void)
{
    /*
     * FreqTest_BackgroundTask 在进入本流程前会先停 DMA，
     * 且时域样本已复制到 s_work_buffer，因此这里可以安全复用共享音频缓冲。
     */
    return (q15_t *)&audio_shared_buf.fft_q15[0];
}

static int64_t fft_bin_magnitude_squared(uint16_t bin)
{
    q15_t *fft_output = fft_output_buffer();
    int32_t real = (int32_t)fft_output[2U * bin];
    int32_t imag = (int32_t)fft_output[(2U * bin) + 1U];

    return ((int64_t)real * real) + ((int64_t)imag * imag);
}

static float fft_bin_magnitude(uint16_t bin)
{
    return sqrtf((float)fft_bin_magnitude_squared(bin));
}

/*============================ 公有函数 ============================*/

void FreqSNR_Init(void)
{
    /* 初始化 RFFT Q15 实例：2048 点，正向 FFT，启用位反转 */
    arm_rfft_init_q15(&s_rfft_inst, 2048, 0, 1);
}

void FreqSNR_Process(const volatile uint16_t *adc_data,
                     uint16_t num_samples,
                     FreqSNR_Result_t *result)
{
    uint16_t i;
    uint16_t process_len = (num_samples < FFT_SIZE) ? num_samples : FFT_SIZE;
    q15_t *fft_output = fft_output_buffer();

    /*---------- Step 1: 计算均值去 DC ----------*/
    int32_t sum = 0;
    for (i = 0; i < process_len; i++)
    {
        sum += (int32_t)adc_data[i];
    }
    int16_t dc_offset = (int16_t)(sum / (int32_t)process_len);

    /*---------- Step 2: 转换为 Q15 + 去 DC + 加窗 ----------
     * ADC 12-bit: 0~4095, 中点 ~2048
     * 去DC后范围: -2048~+2047
     * Q15 满量程 = 32767, 缩放因子 = 32767/2048 ≈ 16.0
     * 直接左移 4 位即可（乘以 16）
     */
    for (i = 0; i < process_len; i++)
    {
        int16_t sample = (int16_t)adc_data[i] - dc_offset;
        q15_t window = hann_window_q15(i);

        s_work_buffer[i] = (q15_t)((int32_t)sample << 4);
        /* 乘以 Hann 窗 */
        s_work_buffer[i] = (q15_t)(((int32_t)s_work_buffer[i] * window) >> 15);
    }
    /* 不足 FFT_SIZE 的部分补零 */
    for (i = process_len; i < FFT_SIZE; i++)
    {
        s_work_buffer[i] = 0;
    }

    /*---------- Step 3: RFFT Q15 ----------
     * 输入: s_work_buffer (in-place 可以，但输出需要独立缓冲)
        * 输出: 复用 audio_shared_buf.fft_q15，格式为 [real0, imag0, real1, imag1, ...]
     */
        arm_rfft_q15(&s_rfft_inst, s_work_buffer, fft_output);

    /*---------- Step 4: 找峰值 bin ----------
     * 跳过 bin 0（DC）和 bin 1（极低频），从 bin 2 开始搜索
     */
    int64_t peak_mag_sq = 0;
    uint16_t peak_bin = 2;
    for (i = 2; i < FFT_NUM_BINS; i++)
    {
        int64_t mag_sq = fft_bin_magnitude_squared(i);

        if (mag_sq > peak_mag_sq)
        {
            peak_mag_sq = mag_sq;
            peak_bin = i;
        }
    }

    /* 检查峰值是否有效（太小说明没有信号） */
    if (peak_mag_sq < (100LL * 100LL)) /* 阈值：Q15 中约 0.003，相当于 ADC 值约 8 */
    {
        result->valid = false;
        result->frequency_hz = 0.0f;
        result->snr_db = 0.0f;
        result->peak_bin = 0;
        result->peak_magnitude = 0.0f;
        return;
    }

    /*---------- Step 5: 抛物线插值提高频率精度 ----------*/
    float mag_left  = (peak_bin > 0) ? fft_bin_magnitude(peak_bin - 1) : 0.0f;
    float mag_center = fft_bin_magnitude(peak_bin);
    float mag_right  = (peak_bin < FFT_NUM_BINS - 1) ? fft_bin_magnitude(peak_bin + 1) : 0.0f;

    float delta = parabolic_interp(mag_left, mag_center, mag_right);

    float freq_resolution = (float)ADC_SAMPLE_RATE_HZ / (float)FFT_SIZE;
    float frequency = ((float)peak_bin + delta) * freq_resolution;

    /*---------- Step 6: 计算 SNR ----------
     * 信号功率 = 峰值 ± SIGNAL_BAND_HALF_WIDTH 范围内的复数幅度平方和
     * 噪声功率 = 其余所有 bin 的复数幅度平方和
     */
    int16_t half_w = SIGNAL_BAND_HALF_WIDTH;
    int64_t signal_power = 0;
    int64_t noise_power = 0;

    for (i = 1; i < FFT_NUM_BINS; i++) /* 跳过 bin 0 (DC) */
    {
        int64_t mag_sq = fft_bin_magnitude_squared(i);

        if ((i >= (peak_bin - half_w)) && (i <= (peak_bin + half_w)))
        {
            signal_power += mag_sq;
        }
        else
        {
            noise_power += mag_sq;
        }
    }

    /* SNR = 10 * log10(signal / noise) */
    float snr_db = 0.0f;
    if (noise_power > 0 && signal_power > 0)
    {
        snr_db = 10.0f * approx_log10f((float)signal_power / (float)noise_power);
    }
    else if (signal_power > 0)
    {
        snr_db = 96.0f; /* 噪声为 0，SNR 极大 */
    }

    /*---------- Step 7: 填充结果 ----------*/
    result->frequency_hz   = frequency;
    result->snr_db        = snr_db;
    result->peak_bin      = peak_bin;
    result->peak_magnitude = sqrtf((float)peak_mag_sq) / 32768.0f;
    result->valid         = true;
}