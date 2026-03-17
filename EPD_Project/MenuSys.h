#ifndef __MENUSYS_H
#define __MENUSYS_H

#include "ti_msp_dl_config.h"
#include "OLED.h"
#include "EPD_GUI.h"
//#include "Pic.h"


typedef struct 
{
    void (*display_func)(uint8_t cursor); // 传入光标位置，让显示函数知道该高亮哪一行
    uint8_t item_count;                   // 当前页面有多少个选项
    uint8_t (*control_func)(uint8_t key, uint8_t cursor); // 处理逻辑并返回新的光标位置
} Menu_Page;

typedef struct 
{
	uint8_t name; 				// 品名不能手动输入，只能预先设定，此变量表示在数据库的引索
	bool if_plus;				// 是否有加量赠送
	uint16_t size1;				// 商品规格
	uint16_t size2;				// 如果有加量，加量规格
	uint8_t size_scale;			// 商品规格单位
	
	uint16_t price1_front;				// 原价整数部分
	uint8_t price1_back;				// 原价小数部分
	uint16_t price2_front;				// 会员价整数部分
	uint8_t price2_back;				// 会员价小数部分
	
	uint16_t pd_year;			// 生产年
	uint8_t pd_month;			// 生产月
	uint8_t pd_day;				// 生产日
	uint8_t duration;			// 有效期
	uint8_t duration_scale;		// 有效期单位
	
} ShopList; // 用于商品信息



#endif