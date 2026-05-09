#ifndef __MENUSYS_H
#define __MENUSYS_H

#include <stdint.h>
#include <stdbool.h>

#include "arm_math.h"
#include "audio_format.h"

/*============================ 宏定义 ============================*/
#define RECORD_MAX_BYTES    (7680000)   // 最大录音量: 16kHz × 240s × 2B
#define BUFFER_HALF_SIZE    (2048)      // 半缓冲 (字)
#define BUFFER_FULL_SIZE    (4096)      // 全缓冲 (字)
#define BLOCK_64KB_SIZE     (65536)     // Flash 64KB 块大小

/* 线性插值上采样 */
#define INTERP_FACTOR            (2)
#define INTERP_CHUNK_RAW_SIZE    (BUFFER_HALF_SIZE / INTERP_FACTOR)   // 1024

/* 全局归一化拉伸 (Q12 定点) */
#define NORM_MAX_GAIN       (8)       /* 最大增益 = 8x (~18dB), 防止弱信号噪声放大 */
#define NORM_SILENCE_RANGE  (50)      /* range < 50 LSB → 视为静音, 输出 0 */
#define NORM_HIGH_RANGE     (3276)    /* range > 80% 满量程 → 不拉伸, 原样输出 */

/*============================ 状态枚举 ============================*/
typedef enum {
    STATE_IDLE = 0,      // 主菜单空闲
    STATE_RECORDING,     // 录音中
    STATE_PLAYING,       // 播放中
    STATE_FREQ_TEST,     // 频率/信噪比测试
    STATE_BYPASS,        // ByPass模式
    STATE_COUNT          // 状态总数（边界保护）
} AppState_enum;

/*============================ 播放模式枚举 ============================*/
typedef enum {
    PLAY_MODE_NORMAL = 0,   // 正常播放
    PLAY_MODE_REVERSE,      // 倒放
} PlaybackMode_enum;

/*============================ 状态结构体 ============================*/
typedef struct {
    void (*on_enter)(void);              // 进入状态时的初始化
    void (*on_exit)(void);               // 退出状态时的清理
    void (*handle_key)(uint8_t key);     // 按键处理
    void (*background_task)(void);       // 主循环后台任务
} AppState;

typedef union {
    uint16_t record[BUFFER_FULL_SIZE];
    uint16_t play[BUFFER_FULL_SIZE];
    q31_t    fft_q31[BUFFER_FULL_SIZE / 2];
    q15_t    fft_q15[BUFFER_FULL_SIZE];
} AudioSharedBuffer_t;

/*============================ 外部声明 ============================*/
extern AppState state_table[];
extern volatile uint8_t g_current_state;

// 全局硬件缓冲与标志（定义在 main.c）
extern volatile AudioSharedBuffer_t audio_shared_buf;
extern volatile uint8_t  flash_write_flag;
extern volatile uint8_t  dac_read_flag;

#define adc_buffer      (audio_shared_buf.record)
#define dac_buffer      (audio_shared_buf.play)

extern uint32_t current_flash_addr;
extern uint32_t playback_flash_addr;
extern uint32_t saved_recording_sample_rate_hz;
extern bool is_filter_on;
extern uint8_t g_playback_mode;  // PlaybackMode_enum
extern uint8_t g_playspeed_mode; // 播放速度倍率（1=正常, 2=2倍速）
extern uint32_t g_current_rec_sample_rate_hz;  // 当前录音采样率（16kHz 或 32kHz）

/* 全局 ADC min/max（录音时追踪，播放时归一化拉伸用） */
extern uint16_t s_global_adc_min;
extern uint16_t s_global_adc_max;

// 频率测试模式变量（定义在 main.c）
extern volatile uint8_t  g_adc_mode;
extern volatile uint8_t  fft_data_ready;

/*============================ 函数声明 ============================*/
void State_Transition(uint8_t new_state);
void FlushFlashBuffer(volatile uint16_t *src, uint16_t half_offset);

#endif