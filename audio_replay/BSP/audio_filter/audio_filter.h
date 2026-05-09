#ifndef __AUDIO_FILTER_H
#define __AUDIO_FILTER_H

#include "ti_msp_dl_config.h"

/* 滤波器级数定义 */
#define FILTER_BIQUAD_STAGES   4     /* 4 级 Biquad（高通1级 + 低通3级） */
#define BUFFER_HALF_SIZE       (2048) /* 与 main.c 中的半缓冲大小保持一致 */

/* 初始化音频滤波器 */
void AudioFilter_Init(void);

/*
 * @brief  对音频数据块进行滤波处理
 * @param  pInput   输入数据缓冲区（ADC 原始数据，uint16 格式）
 * @param  pOutput  输出数据缓冲区（滤波后数据，uint16 格式）
 * @param  numSamples  样本数（如 BUFFER_HALF_SIZE = 2048）
 */
void AudioFilter_Process(const volatile uint16_t *pInput,
                         volatile uint16_t *pOutput,
                         uint32_t numSamples);

#endif /* __AUDIO_FILTER_H */