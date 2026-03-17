#include "WaveProcess.h"

void Process_Waveform(volatile uint16_t* buffer, uint16_t length, WaveResult_t* result) 
{
    uint32_t sum_all = 0;
    uint16_t max_adc = 0;
    uint16_t min_adc = 4095;
    
    // ==========================================
    // 第一轮：全局特征提取 (用于初步判断)
    // ==========================================
    for (int i = 0; i < length; i++) 
    {
        uint16_t val = buffer[i];
        if (val > max_adc) max_adc = val;
        if (val < min_adc) min_adc = val;
        sum_all += val;
    }
    
    uint16_t adc_vpp = max_adc - min_adc;
    result->Vpp = (adc_vpp * VREF) / ADC_RES; // 初步 Vpp
    
    // 直流拦截：若幅值过小，视为直流或纯噪声
    if (result->Vpp < 0.04f) 
    {
        result->Type = WAVE_DC;
        result->Freq = 0.0f;
        result->Duty = 0.0f;
        result->Vdc = ((float)sum_all / length * VREF) / ADC_RES;
        result->Vrms = result->Vdc;
        return; 
    }

    // ==========================================
    // 第二轮：边缘检测与特征统计
    // ==========================================
    // 识别专用带宽 (20%-80% Vpp)：用于区分正弦和三角波
    uint16_t id_upper = max_adc - (adc_vpp / 5); 
    uint16_t id_lower = min_adc + (adc_vpp / 5);
    
    // 触发专用带宽 (30%-70% Vpp)：施密特触发器增加抗噪性
    uint16_t mid_adc = (max_adc + min_adc) / 2;
    uint16_t trig_high = mid_adc + (adc_vpp / 5); 
    uint16_t trig_low  = mid_adc - (adc_vpp / 5);

    int mid_zone_count = 0;
    int first_rise = -1, last_rise = -1, first_fall = -1;
    int cycle_count = 0;
    uint8_t state = (buffer[0] > mid_adc) ? 1 : 0;

    for (int i = 0; i < length; i++) 
    {
        // 统计落入 ID 带宽的点数
        if (buffer[i] > id_lower && buffer[i] < id_upper) mid_zone_count++;

        // 状态机寻找过零点 (施密特触发)
        if (i > 0) {
            if (state == 0 && buffer[i] > trig_high) {
                state = 1;
                if (first_rise == -1) first_rise = i;
                last_rise = i;
                cycle_count++;
            } 
            else if (state == 1 && buffer[i] < trig_low) {
                state = 0;
                if (first_rise != -1 && first_fall == -1) first_fall = i;
            }
        }
    }

    // ==========================================
    // 第三轮：波形类型识别
    // ==========================================
    float mid_ratio = (float)mid_zone_count / length;
    if (mid_ratio < 0.20f)       result->Type = WAVE_SQUARE;    // 方波：中间段停留极短
    else if (mid_ratio < 0.52f)  result->Type = WAVE_SINE;      // 正弦波：理论值约 41%
    else                         result->Type = WAVE_TRIANGLE;  // 三角波：理论值约 60%

    // ==========================================
    // 第四轮：全周期精确计算与方波专项优化
    // ==========================================
    if (cycle_count >= 2 && first_rise != -1) 
    {
        uint64_t p_sum = 0;
        uint64_t p_sq_sum = 0;
        uint64_t high_sum = 0, low_sum = 0;
        uint32_t high_cnt = 0, low_cnt = 0;
        uint32_t w_len = last_rise - first_rise;

        for (int i = first_rise; i < last_rise; i++) 
        {
            uint32_t v = buffer[i];
            p_sum += v;
            p_sq_sum += (uint64_t)v * v;
            
            // 为方波准备：高低电平分离统计
            if (v > mid_adc) {
                high_sum += v;
                high_cnt++;
            } else {
                low_sum += v;
                low_cnt++;
            }
        }

        // 基础计算（物理真平均，适用于正弦/三角波）
        result->Vdc = ((float)p_sum / w_len * VREF) / ADC_RES;
        result->Vrms = (sqrtf((float)p_sq_sum / w_len) * VREF) / ADC_RES;
        
        float avg_period = (float)w_len / (cycle_count - 1);
        result->Freq = SAMPLE_RATE / avg_period;
        
        if (first_fall > first_rise) 
		{
            result->Duty = ((float)(first_fall - first_rise) / avg_period) * 100.0f;
        } 
		
		else 
		{
            result->Duty = 50.0f;
        }

        // --- 方波专项修正：对齐信号发生器 Offset 定义 ---
        if (result->Type == WAVE_SQUARE && high_cnt > 0 && low_cnt > 0) 
        {
            float v_top = ((float)high_sum / high_cnt * VREF) / ADC_RES;
            float v_base = ((float)low_sum / low_cnt * VREF) / ADC_RES;
            
            // 重新定义 Vdc 为几何中点，确保不随占空比漂移
            result->Vdc = (v_top + v_base) / 2.0f;
            // 重新定义 Vpp 为平顶间距，自动消除过冲（振铃）误差
            result->Vpp = v_top - v_base;
        }
    } 
    else 
    {
        // 降级处理：无法捕捉周期时回退到全局统计
        result->Vdc = ((float)sum_all / length * VREF) / ADC_RES;
        result->Vrms = result->Vdc; 
        result->Freq = 0.0f;
        result->Duty = 0.0f;
    }
}

// 小信号专属宏定义
#define VREF_SMALL 3.3f      // 内部参考电压改为了 2.5V
#define ADC_RES    4096.0f   // 12位 ADC
#define SAMPLE_RATE 100000.0f // 100kHz 采样率

void Process_Small_Signal(volatile uint16_t* buffer, uint16_t length, WaveResult_t* result) 
{
    uint32_t sum = 0;
    
    // ==========================================
    // Pass 1: 第一遍遍历，仅计算纯直流偏置 (DC)
    // ==========================================
    for (int i = 0; i < length; i++) 
    {
        sum += buffer[i]; 
    }
    
    // 计算出极为精确的中心轴 (浮点形式的 ADC 刻度)
    float vdc_adc = (float)sum / length;
    result->Vdc = (vdc_adc * VREF_SMALL) / ADC_RES;
    
    // ==========================================
    // Pass 2: 第二遍遍历，拔除直流，专算交流 (AC RMS)
    // ==========================================
    float ac_sq_sum = 0.0f;
    for (int i = 0; i < length; i++) 
    {
        // 每个点减去均值，得到纯交流分量 (正负摆动，数值很小，在 -25 到 +25 之间)
        float ac_val = (float)buffer[i] - vdc_adc; 
        
        // 此时平方相加，完全没有"大数吃小数"的危险
        ac_sq_sum += (ac_val * ac_val);
    }
    
    // 计算交流有效值 (ADC刻度下的 RMS)
    float ac_rms_adc = sqrtf(ac_sq_sum / length);
    
    // 转换为真实电压
    float ac_rms_volts = (ac_rms_adc * VREF_SMALL) / ADC_RES;
    result->Vrms = ac_rms_volts; 
    
    // 正弦波理论反推 Vpp
    result->Vpp = 2.828427f * ac_rms_volts; 
    
    result->Type = WAVE_SINE;
    result->Duty = 50.0f;

    // ==========================================
    // Pass 3: 频率计算 (依然使用精确的 vdc_adc 做中心阈值)
    // ==========================================
    uint16_t thresh_high = (uint16_t)(vdc_adc + 3); 
    uint16_t thresh_low  = (uint16_t)(vdc_adc - 3);

    int cycle_count = 0;
    int first_rise = -1, last_rise = -1;
    uint8_t state = (buffer[0] > vdc_adc) ? 1 : 0;

    for (int i = 1; i < length; i++) 
    {
        if (state == 0 && buffer[i] > thresh_high) 
        {
            state = 1;
            if (first_rise == -1) first_rise = i;
            last_rise = i;
            cycle_count++;
        } 
        else if (state == 1 && buffer[i] < thresh_low) 
        {
            state = 0;
        }
    }

    if (cycle_count >= 2 && first_rise != -1) 
    {
        float total_period = (float)(last_rise - first_rise);
        float avg_period = total_period / (cycle_count - 1);
        result->Freq = SAMPLE_RATE / avg_period;
    } 
    else 
    {
        result->Freq = 0.0f; 
    }
}

#ifndef PI
#define PI 3.14159265358979323846f
#endif

/**
 * @brief 使用 Goertzel 算法计算特定频率的幅值
 * @param buffer 采样数据缓冲区
 * @param target_f 目标探测频率 (Hz)
 * @param fs 采样率 (100000.0f)
 * @param n 采样点数 (2048)
 * @return 该频率对应的电压幅值 (V)
 */
float Get_Specific_Freq_Mag(volatile uint16_t* buffer, float target_f, float fs, int n, float vdc_adc_raw) 
{
    float k = (n * target_f) / fs;
    float omega = (2.0f * PI * k) / n;
    float cosine = cosf(omega);
    float coeff = 2.0f * cosine;
    
    float q0, q1 = 0.0f, q2 = 0.0f;
    
    // 核心循环
    for (int i = 0; i < n; i++) 
    {
        // 动态减去真实直流分量，消除 1.5V 或 1V 偏移带来的误差
        float x = ((float)buffer[i] - vdc_adc_raw) * (3.3f / 4096.0f); 
        q0 = x + coeff * q1 - q2;
        q2 = q1;
        q1 = q0;
    }
    
    float magnitude = sqrtf(q1 * q1 + q2 * q2 - q1 * q2 * coeff);
    return (magnitude * 2.0f / n);
}