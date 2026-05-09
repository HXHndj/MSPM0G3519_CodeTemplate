/**
 * @file    audio_filter.c
 * @brief   基于 CMSIS-DSP 的 IIR Biquad 级联音频滤波器 (Q31 实现)
 *
 * 滤波器设计参数：
 *   采样率：16 kHz
 *   Stage 1: 高通 Butterworth, fc = 80 Hz, 2阶 (1级 Biquad) — 去直流+低频噪声
 *   Stage 2~4: 低通 Butterworth, fc = 7.2 kHz, 6阶 (3级 Biquad) — 抗混叠+高频抑制
 *
 * 采用 Q31 定点格式 (32-bit)，量化精度远优于 Q15，避免 80Hz 高通极点量化不稳定。
 * 系数经 Python scipy.signal.iirfilter 计算，postShift=1 补偿超出 [-1,1) 的系数。
 *
 * Cortex-M0+ 天然是 32 位内核，处理 Q31 不比 Q15 慢多少。
 */

#include "audio_filter.h"
#include "arm_math.h"

#define AUDIO_FILTER_INTERNAL_CHUNK_SAMPLES   (128U)

/*================================================================
 * 滤波器系数 (Q31 格式, postShift = 1)
 *
 * CMSIS Biquad 系数排列: [b0, b1, b2, -a1, -a2]  (a0 = 1 不存储)
 * 所有系数已除以 2，运行时由 postShift=1 左移补偿回来。
 *
 * 由 calc_filter.py 用 scipy.signal.iirfilter 生成:
 *   sos_hp = iirfilter(2, 80, btype='high', fs=16000, output='sos')
 *   sos_lp = iirfilter(6, 7200, btype='low', fs=16000, output='sos')
 *===============================================================*/
static const q31_t filterCoeffs[FILTER_BIQUAD_STAGES * 5] = {
    /* Stage 1 - High-pass fc=80Hz, 2nd order Butterworth */
     1050152231, -2100304461,  1050152231,  2099786147, -1027080952,
    /* Stage 2 - Low-pass fc=7.2kHz, section 1 of 3 */
      583361503,  1166723007,   583361503, -1572890246,  -580092853,
    /* Stage 3 - Low-pass fc=7.2kHz, section 2 of 3 */
     1073741824,  2147483647,  1073741824, -1676130396,  -688645970,
    /* Stage 4 - Low-pass fc=7.2kHz, section 3 of 3 */
     1073741824,  2147483647,  1073741824, -1891126960,  -914706735,
};

/* Biquad 状态缓冲 — 每级 4 个 q31_t 状态变量 */
static q31_t filterState[FILTER_BIQUAD_STAGES * 4];

/* CMSIS Biquad Q31 实例 */
static arm_biquad_casd_df1_inst_q31 biquadInstance;

/* 内部 Q31 分块缓冲，避免占用整块半缓冲 RAM */
static q31_t tempInput[AUDIO_FILTER_INTERNAL_CHUNK_SAMPLES];
static q31_t tempOutput[AUDIO_FILTER_INTERNAL_CHUNK_SAMPLES];

/*-------------------------------------------------------------
 * AudioFilter_Init — 初始化 Biquad 滤波器
 *-------------------------------------------------------------*/
void AudioFilter_Init(void)
{
    /*
     * postShift = 1: 系数存储时已除以 2，
     * 每次运算后硬件自动左移 1 位补偿增益。
     */
    arm_biquad_cascade_df1_init_q31(
        &biquadInstance,
        FILTER_BIQUAD_STAGES,
        (q31_t *)filterCoeffs,
        filterState,
        1                  /* postShift = 1 */
    );
}

/*-------------------------------------------------------------
 * AudioFilter_Process — 对一个半缓冲块执行滤波
 *
 * 数据流：
 *   uint16 ADC (0~4095)
 *     → 减去中点 2048，左移 20 位 → Q31 [-1.0, ~1.0)
 *     → Biquad 滤波
 *     → 右移 20 位，加回 2048 → uint16 (0~4095)
 *     → 限幅保护
 *-------------------------------------------------------------*/
void AudioFilter_Process(const volatile uint16_t *pInput,
                         volatile uint16_t *pOutput,
                         uint32_t numSamples)
{
    uint32_t offset = 0;

    while (offset < numSamples)
    {
        uint32_t blockSize = numSamples - offset;
        uint32_t i;

        if (blockSize > AUDIO_FILTER_INTERNAL_CHUNK_SAMPLES)
        {
            blockSize = AUDIO_FILTER_INTERNAL_CHUNK_SAMPLES;
        }

        /*
         * 1. uint16 ADC → Q31
         *    ADC 12-bit 无符号: 0 ~ 4095
         *    减去中点 2048 → 有符号: -2048 ~ 2047
         *    左移 20 位对齐到 Q31 满量程:
         *      2047 << 20 = 0x7FE00000 ≈ +0.999 (Q31)
         *     -2048 << 20 = 0x80000000   = -1.000 (Q31)
         */
        for (i = 0; i < blockSize; i++)
        {
            tempInput[i] = ((q31_t)pInput[offset + i] - 2048) << 20;
        }

        /*
         * 2. 执行 Biquad 级联滤波 (Q31, 2^postShift 补偿)
         */
        arm_biquad_cascade_df1_q31(
            &biquadInstance,
            tempInput,
            tempOutput,
            blockSize
        );

        /*
         * 3. Q31 → uint16 (DAC 输出)
         *    右移 20 位恢复幅度，加回中点偏移 2048
         *    限幅保护防止 DAC 溢出 (0 ~ 4095)
         */
        for (i = 0; i < blockSize; i++)
        {
            int32_t val = (tempOutput[i] >> 20) + 2048;
            if (val < 0)    val = 0;
            if (val > 4095) val = 4095;
            pOutput[offset + i] = (uint16_t)val;
        }

        offset += blockSize;
    }
}