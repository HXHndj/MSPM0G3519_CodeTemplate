#ifndef __WAVEPROCESS_H
#define __WAVEPROCESS_H

#include <math.h>
#include "ti_msp_dl_config.h"

// ADC 参考电压和分辨率 (请根据实际情况修改，如果用了内部 2.5V 建议改掉 3.3f)
#define VREF 3.3f      
#define ADC_RES 4096.0f 
#define SAMPLE_RATE 100000.0f // 100kHz 采样率

typedef struct 
{
    float Harmonics[5]; // 0:基波, 1:二次谐波... 4:五次谐波
} FFTResult_t;

typedef enum 
{
    WAVE_UNKNOWN = 0,
    WAVE_DC,        // <--- 新增：纯直流或底噪
    WAVE_SQUARE,
    WAVE_SINE,
    WAVE_TRIANGLE
} WaveType_t;

typedef struct 
{
    float Vpp;          // 峰峰值
    float Vdc;          // 直流偏置 (平均值)
    float Vrms;         // 有效值
    float Freq;         // 频率 (Hz)
    float Duty;         // 占空比 (%)
    WaveType_t Type;    // 波形类型
} WaveResult_t;

void Process_Waveform(volatile uint16_t* buffer, uint16_t length, WaveResult_t* result);
void Process_Small_Signal(volatile uint16_t* buffer, uint16_t length, WaveResult_t* result);
float Get_Specific_Freq_Mag(volatile uint16_t* buffer, float target_f, float fs, int n, float vdc_adc_raw);

#endif