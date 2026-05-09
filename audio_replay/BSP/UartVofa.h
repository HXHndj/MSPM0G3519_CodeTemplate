#ifndef _UARTVOFA_H_
#define _UARTVOFA_H_

#include "bsp.h"

/* VOFA+ JustFloat 协议帧尾 (IEEE 754 +inf) */
#define VOFA_FRAME_FOOTER0  0x00
#define VOFA_FRAME_FOOTER1  0x00
#define VOFA_FRAME_FOOTER2  0x80
#define VOFA_FRAME_FOOTER3  0x7F

#define UART_TX_BUF_SIZE    (512)     // 串口发送缓冲大小

/* VOFA+ 发送函数声明 */
void SendAudioToVofaPlus(void);
void SendAudioVoltageToVofaPlus(void);

#endif