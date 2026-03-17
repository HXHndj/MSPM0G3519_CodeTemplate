#include "ti_msp_dl_config.h"
#include "bsp.h"
#include "KeyBroad.h"
#include "delay.h"
#include "EPD_GUI.h"
#include "OLED.h"
#include "MenuSys.h"
#include "Pic.h"
#include "EEPROM_WR.h"

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
	
	墨水屏幕的引脚：
	因为底板上的SPI引脚接了OLED屏幕，故用了另一个SPI引脚
	RES -> PB4
	CS -> PB1
	DC -> PB5
	BUSY -> PB0
	SPI1
	SCLK -> PB16
	PICO (SDA) -> PB15
	
*/

extern uint8_t g_CursorPos; // 全局变量：光标位置（0表示第一行）
extern uint8_t g_PageIndex; // 全局变量：当前页面ID
extern Menu_Page table[];
extern bool OLED_Updateflag;
extern ShopList Product;

//uint8_t BLE_RecFlag = 0; // 开始读取数据包的标志位
//char BLE_RecBuf[32] = {0};
//uint8_t BLE_RecPos = 0; 
//uint8_t Datalen = 0;
//uint8_t BLE_DataReady = 0; // 数据接收完成标志，避免中断内操作OLED

int main(void)
{
	uint8_t KeyNum = 0;
	
    SYSCFG_DL_init();
    EPD_GPIOInit();
	OLED_Init();
	NVIC_EnableIRQ(TIMER_KEY_INST_INT_IRQN);
//	NVIC_EnableIRQ(UART_BLE_INST_INT_IRQN);
	DL_TimerA_startCounter(TIMER_KEY_INST);
	
    Flash_EEPROM_Init();          // 初始化 Flash 仿真
    Flash_LoadProduct(&Product);  // 从 Flash 读取上次保存的商品数据
    
    // 如果是第一次运行，Flash 里全是乱码(0xFF)，则赋默认值并保存一次
	if (Product.pd_year == 0xFFFF || Product.pd_year == 0 || Product.name > 3) 
    {
        ShopList default_product = {0,1,150,30,1,688,88,388,88,2024,1,1,12,0};
        Product = default_product;
        Flash_SaveProduct(&Product, &EEPROMEmulationState);
    }

	OLED_Clear();
	OLED_ShowImage(0, 0, 128, 64, Ark);
	OLED_Update();
	delay_ms(500);
	//Product.if_plus = 1;
	while (1)
	{
		KeyNum = GetKeyNum_NonBlocking();
		
		if (KeyNum != 0) 
		{
			if (table[g_PageIndex].control_func != NULL) // 确保当前页面有控制函数
			{
				table[g_PageIndex].control_func(KeyNum, g_CursorPos); // 调用控制逻辑，更新光标或处理跳转
			}
		}
		
		if ((table[g_PageIndex].display_func != NULL) && (OLED_Updateflag == 1)) // 显示模块
		{
			table[g_PageIndex].display_func(g_CursorPos); 
//			OLED_ShowNum(112,0,g_PageIndex,1,OLED_6X8);
//			OLED_ShowNum(112,8,g_CursorPos,2,OLED_6X8);
			OLED_Update();
			OLED_Updateflag = 0;
		}
		
//		if (BLE_DataReady == 1)
//		{
//			// 清除将要显示文本的区域 (假设显示在 x=0, y=48 的位置，高16像素)
//			OLED_Clear(); 
//			
//			// 使用 8x16 字体显示接收到的中文字符串
//			OLED_ShowString(0, 48, BLE_RecBuf, OLED_8X16);
//			
//			// 局部刷新该区域，或者直接设置 OLED_Updateflag = 1 交给上面的显示逻辑刷新
//			OLED_Update();
//			
//			BLE_DataReady = 0; // 清除标志位，等待下一次接收
//		}
	}
}

// 蓝牙模块 待拓展
/*
	引脚配置 UART4
	蓝牙RX -> 单片机TX PB10
	蓝牙TX -> 单片机RX PB11
*/ 

//void UART_BLE_INST_IRQHandler(void)
//{
//	uint8_t rx_data;
//	switch (DL_UART_Main_getPendingInterrupt(UART_BLE_INST))
//	{
//		case DL_UART_MAIN_IIDX_RX:
//			rx_data = DL_UART_Main_receiveData(UART_BLE_INST); 
//			DL_GPIO_togglePins(LED_PIN_1_PORT, LED_PIN_1_PIN);

//			if (rx_data == '[')
//			{
//				BLE_RecFlag = 1; // 标志开启读取数据包
//				BLE_RecPos = 0; 
//				Datalen = 0;
//				BLE_DataReady = 0; 
//				memset(BLE_RecBuf, 0, sizeof(BLE_RecBuf));
//			}
//			else if (rx_data == ']')
//			{
//				if (BLE_RecFlag == 1) // 确保有对应的 '['
//				{
//					BLE_RecFlag = 0; // 标志数据包读取停止
//					BLE_RecBuf[BLE_RecPos] = '\0'; // 关键：添加字符串结束符
//					Datalen = BLE_RecPos;
//					BLE_RecPos = 0;
//					BLE_DataReady = 1; // 标记数据就绪，主循环处理显示
//				}
//			}
//			else if (BLE_RecFlag == 1)
//			{
//				// 接收所有字符（包含汉字的 GBK 字节）
//				// 防止数组越界，预留最后一位给 '\0'
//				if (BLE_RecPos < sizeof(BLE_RecBuf) - 1) 
//				{
//					BLE_RecBuf[BLE_RecPos] = rx_data; // 直接存入字符本身
//					BLE_RecPos++;
//				}
//			}
//			break;
//		default: break;
//	}
//}
