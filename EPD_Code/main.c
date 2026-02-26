#include "ti_msp_dl_config.h"
#include "bsp.h"
#include "KeyBroad.h"
#include "delay.h"
#include "EPD_GUI.h"
#include "Pic.h"

/*
	此为墨水屏模块测试程序，相关代码已经成功进行移植
	由于此墨水屏模块为三色显示，刷新速度过慢，只能用于做静态内容显示
	引脚配置
	GND,VCC提供供电
	SCL,SDA使用硬件SPI的引脚连接，此处直接连在原OLED屏处位置
	RES,DC,CS需要手动用GPIO配置，皆为输出
	BUSY需要配置为输入，悬空，当墨水屏处于刷新状态，此引脚为高电平
	
	硬件SPI，比特率选在4M到10M之间
	Frame Format 	Motorola 3-wire
	Clock Polarity 	Low
	Phase 			Data captured on first clock edge
	Frame Size 		8 (bits)
	Bit Order 		MSB
*/

uint8_t KeyNum = 0;
uint8_t BlackImage[2888]; // 存放黑色内容的缓冲区
uint8_t RedImage[2888];   // 存放红色内容的缓冲区

int main(void)
{
    SYSCFG_DL_init();
    EPD_GPIOInit();
	
	// 1. 硬件初始化
    EPD_Init();
    EPD_Display_Clear(); // 清除屏幕电荷（物理清屏）

	// 在绘制之前，要先将两个画布全部清空
	// 初始化黑白层画布
	Paint_NewImage(BlackImage, 152, 152, 0, WHITE);
	Paint_SelectImage(BlackImage);
	Paint_Clear(WHITE); // 背景涂白

	// 初始化红色层画布
	Paint_NewImage(RedImage, 152, 152, 0, WHITE);
	Paint_SelectImage(RedImage);
	Paint_Clear(WHITE); // 背景涂白

	// --- 绘制黑色部分 ---
	Paint_SelectImage(BlackImage);
//	EPD_ShowString(0, 0, "This is Black", 16, BLACK);
//	EPD_DrawRectangle(10, 30, 140, 50, BLACK, 0); // 黑色边框
	EPD_ShowPicture(0,0,152,152,gImage_cqyb,BLACK);

	// --- 绘制红色部分 ---
	Paint_SelectImage(RedImage);
	// 注意：在 RedImage 上画 BLACK，最终在屏幕上会显示为红色
//	EPD_ShowString(0, 0, "TESTTTTTTT", 16, BLACK);
//	EPD_ShowString(10, 60, "This is Red", 16, BLACK);
//	EPD_DrawCircle(76, 100, 20, BLACK, 1); // 红色实心圆
	EPD_ShowPicture(0,0,152,152,gImage_cqyr,BLACK);
	
	// 发送双层数据
	// 第一个参数对应寄存器 0x24 (黑白)，第二个参数对应寄存器 0x26 (红色)
	EPD_Display(BlackImage, RedImage);

	// 触发屏幕物理刷新
	EPD_Update();

	// 刷新完成后休眠
	EPD_DeepSleep();
}
