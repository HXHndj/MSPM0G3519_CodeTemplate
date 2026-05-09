/**
 * @file    freq_snr_test.h
 * @brief   基于 CMSIS-DSP RFFT Q15 的频率与信噪比测量模块
 *
 * 使用方法：
 *   1. 调用 FreqSNR_Init() 初始化（上电一次）
 *   2. DMA 采集 ADC 数据到 fft_buffer 后，调用 FreqSNR_Process()
 *   3. 从 FreqSNR_Result_t 获取频率、SNR 等结果
 */

#ifndef __FREQ_SNR_TEST_H
#define __FREQ_SNR_TEST_H

#include <stdint.h>
#include <stdbool.h>
#include "ti_msp_dl_config.h"

/*============================ 参数配置 ============================*/

#define FFT_SIZE                2048    /* FFT 点数（与半缓冲大小一致） */
#define FFT_NUM_BINS            (FFT_SIZE / 2)  /* 有效频率 bin 数 */
#define ADC_SAMPLE_RATE_HZ      16000   /* ADC 采样率 (Hz) */
#define SIGNAL_BAND_HALF_WIDTH  3       /* 信号带宽（峰值 ± 此值视为信号） */

/*============================ 结果结构体 ============================*/

typedef struct {
    float    frequency_hz;    /* 检测到的信号频率 (Hz) */
    float    snr_db;          /* 信噪比 (dB) */
    uint16_t peak_bin;        /* 峰值所在 FFT bin 索引 */
    float    peak_magnitude;  /* 峰值幅度（归一化） */
    bool     valid;           /* 结果是否有效 */
} FreqSNR_Result_t;

/*============================ 函数声明 ============================*/

/**
 * @brief  初始化频率/信噪比测试模块（上电调用一次）
 *         初始化 RFFT 实例 + 预计算 Hann 窗
 */
void FreqSNR_Init(void);

/**
 * @brief  对采集到的 ADC 数据进行频率与 SNR 分析
 * @param  adc_data   ADC 原始数据缓冲（uint16, 0~4095）
 * @param  numSamples 样本数量（应 >= FFT_SIZE，取前 FFT_SIZE 个）
 * @param  result     输出分析结果
 */
void FreqSNR_Process(const volatile uint16_t *adc_data,
                     uint16_t num_samples,
                     FreqSNR_Result_t *result);

#endif /* __FREQ_SNR_TEST_H */