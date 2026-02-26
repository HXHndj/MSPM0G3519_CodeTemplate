#ifndef __ZW101_H__
#define __ZW101_H__

#include "ti_msp_dl_config.h"
#include "delay.h"
#include <stdint.h>
#include <stdbool.h>

#define ZW101_UART_INST      ZW_Uart_INST

// 指纹模块指令码定义 
#define ZW101_CMD_GET_IMAGE  		0x01 // 获取图像
#define ZW101_CMD_GEN_CHAR   		0x02 // 生成特征
#define ZW101_CMD_MATCH      		0x03 // 精确比对
#define ZW101_CMD_SEARCH     		0x04 // 搜索指纹
#define ZW101_CMD_REG_MODEL  		0x05 // 合并特征
#define ZW101_CMD_STORE_CHAR 		0x06 // 存储模板
#define ZW101_CMD_DELETE     		0x0C // 删除模板
#define ZW101_CMD_CLEAR_LIB  		0x0D // 清空指纹库
#define ZW101_CMD_LED_CTRL   		0x3C // 呼吸灯控制
#define ZW101_CMD_VALID_TEMP_NUM 	0x1D // 读有效模板个数
#define ZW101_CMD_READ_INDEX 		0x1F // 读索引表指令
#define ZW101_CMD_CANCEL 	 		0xAA // 过程终止指令

// LED 颜色定义
#define ZW101_LED_OFF        0x00
#define ZW101_LED_BLUE       0x01
#define ZW101_LED_GREEN      0x02
#define ZW101_LED_RED        0x04

// LED 模式定义
#define ZW101_LED_BREATH     0x01
#define ZW101_LED_FLASH      0x02
#define ZW101_LED_ON         0x03

// 模块状态返回码
#define ZW101_ACK_SUCCESS    0x00
#define ZW101_ACK_NO_FINGER  0x02

// 函数声明
void ZW101_Init(void);
bool ZW101_Enroll(uint16_t id, uint8_t times);
bool ZW101_Match(uint16_t *matched_id);
bool ZW101_Delete(uint16_t id);
bool ZW101_Clear(void);
void ZW101_LedCtrl(uint8_t mode, uint8_t color, uint8_t count);
uint8_t ZW101_Identify(uint16_t *matched_id);
bool ZW101_GetEnrollCount(uint16_t *count);
bool ZW101_GetFreeID(uint16_t *free_id);
bool ZW101_Cancel(void);
bool ZW101_CheckEnrolled(uint16_t id);

#endif