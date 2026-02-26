#include "ti_msp_dl_config.h"
#include "bsp.h"
#include "KeyBroad.h"
#include "OLED.h"
#include "delay.h"

#include <stdbool.h>
#include <stdint.h>
#include "qrcodegen.h"

uint8_t KeyNum = 0;
uint8_t BLE_RecFlag = 0; // 开始读取数据包的标志位
uint8_t BLE_RecBuf[64] = {0};
uint8_t BLE_RecPos = 0; // 0到63
uint8_t Datalen = 0;
uint8_t BLE_DataReady = 0; // 数据接收完成标志，避免中断内操作OLED

/**
 * @brief 把qrcodegen库生成的二维码更新到OLED缓存区，使用后要调用Update函数
 * @param qrcode: 库生成的二维码数组（qr0）
 * @param start_x: 显示起始X坐标
 * @param start_y: 显示起始Y坐标
 * @param scale: 缩放倍数（推荐2/3，适配0.96/1.3寸OLED）
 */
void QRCode_DrawToOLED(const uint8_t qrcode[], int16_t start_x, int16_t start_y, uint8_t scale) 
{
    int size = qrcodegen_getSize(qrcode); // 获取二维码边长（版本1=21）
    
    // 遍历每个二维码像素
    for (int y = 0; y < size; y++) 
	{
        for (int x = 0; x < size; x++) 
		{
            // qrcodegen_getModule返回true=黑色（需要点亮OLED像素），false=白色
            if (qrcodegen_getModule(qrcode, x, y)) 
				{
                // 缩放显示：每个二维码像素对应scale×scale的OLED像素
                for (uint8_t dy = 0; dy < scale; dy++) 
					{
                    for (uint8_t dx = 0; dx < scale; dx++) 
						{
                        int16_t px = start_x + x * scale + dx;
                        int16_t py = start_y + y * scale + dy;
                        // 适配1.3寸OLED=132×64，0.96寸=128×64
                        if (px >= 0 && px < 132 && py >= 0 && py < 64) 
						{
                            OLED_DrawPoint(px, py); // 绘制像素
                        }
                    }
                }
            }
        }
    }
}

int main(void)
{
    SYSCFG_DL_init();
    OLED_Init();
	NVIC_EnableIRQ(UART_BLE_INST_INT_IRQN);
//	NVIC_EnableIRQ(TIMER2_INST_INT_IRQN);
//	DL_TimerA_startCounter(TIMER2_INST);
	
	OLED_Clear();
    OLED_ShowImage(0, 0, 128, 64, Ark);
    OLED_Update();
    delay_ms(500);
    OLED_Clear();
    OLED_Update();
    
	// 具体实现函数参考Github上的代码案例，qr0用于二维码存储，tempBuffer用于临时缓存，数组长度直接按照最大的数据长度，具体数值可看头文件
	uint8_t qr0[qrcodegen_BUFFER_LEN_MAX];
	uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
//	bool ok = qrcodegen_encodeText("This is a QRcode Generator",
//		tempBuffer, qr0, qrcodegen_Ecc_MEDIUM,
//		qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX,
//		qrcodegen_Mask_AUTO, true);
//	if (!ok) //错误处理
//	{
//		return 0;
//	}

//	int size = qrcodegen_getSize(qr0);
//	QRCode_DrawToOLED(qr0,0,0,2);
//	OLED_Update();
	
    while (1) 
    {
		if (BLE_DataReady)
		{
			OLED_Clear();
			bool ok = qrcodegen_encodeText(BLE_RecBuf,
			tempBuffer, qr0, qrcodegen_Ecc_MEDIUM,
			qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX,
			qrcodegen_Mask_AUTO, true);
			if (!ok) //错误处理
			{
				return 0;
			}

			int size = qrcodegen_getSize(qr0);
			QRCode_DrawToOLED(qr0,0,0,2);
//			OLED_Update();
			OLED_ShowString(0,54,BLE_RecBuf,OLED_6X8);
			OLED_Update();
			BLE_DataReady = 0;
		}
//      KeyNum = GetKeyNum();
//		if (KeyNum != 0)
//		{
//			switch (KeyNum)
//			{
//				case 1:DL_UART_transmitData(UART_BLE_INST,'9');break;
//				case 2:DL_UART_transmitDataBlocking(UART_BLE_INST,'1');break;
//			}
//		}
//		
//		OLED_Update();
	}
}

// 数据包的结构为 [data]
void UART_BLE_INST_IRQHandler()
{
	// 必须读取接收数据，清空RX FIFO
	uint8_t rx_data = DL_UART_Main_receiveData(UART_BLE_INST); 
	
	if (rx_data == '[')
	{
		BLE_RecFlag = 1; // 标志开启读取数据包
		BLE_RecPos = 0; Datalen = 0;
        BLE_DataReady = 0; // 重置就绪标志
		memset(BLE_RecBuf, 0, sizeof(BLE_RecBuf));
	}
	
	else if (rx_data == ']')
	{
		BLE_RecFlag = 0; // 标志数据包读取停止
		Datalen = BLE_RecPos;
		BLE_RecPos = 0;
		if (Datalen>0){ BLE_DataReady = 1;}
	}
	
	else if (BLE_RecFlag == 1)
	{
		if ((rx_data >= 'a' && rx_data <= 'z') || 
            (rx_data >= 'A' && rx_data <= 'Z') || 
            (rx_data >= '0' && rx_data <= '9') || 
		rx_data == '.' || rx_data == '/' || rx_data == ':' || rx_data == ' ')
		{
			if (BLE_RecPos >= 63) // 先判断是否越界，再赋值
			{ // 缓冲区下标0-63，最大63
				BLE_RecFlag = 0;BLE_RecPos = 0;BLE_DataReady = 0;// 越界重置
				memset(BLE_RecBuf, 0, sizeof(BLE_RecBuf));Datalen = 0;
			} 
			else 
			{
				BLE_RecBuf[BLE_RecPos] = rx_data; // 存储字符本身
				BLE_RecPos++;
			}
		}
		else {BLE_RecFlag = 0;BLE_RecPos = 0;Datalen = 0;BLE_DataReady = 0;memset(BLE_RecBuf, 0, sizeof(BLE_RecBuf));}
	}
	DL_GPIO_togglePins(LED_PIN_1_PORT, LED_PIN_1_PIN);
//  L_UART_clearInterruptStatus(UART_BLE_INST, DL_UART_MAIN_INTERRUPT_RX);
}