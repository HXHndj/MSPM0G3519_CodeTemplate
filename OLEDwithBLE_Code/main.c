#include "ti_msp_dl_config.h"
#include "bsp.h"
#include "KeyBroad.h"
#include "OLED.h"
#include "delay.h"

uint8_t KeyNum = 0;
uint8_t BLE_RecFlag = 0; // 开始读取数据包的标志位
uint8_t BLE_RecBuf[16] = {0};
uint8_t BLE_RecPos = 0; // 0到15
uint8_t Datalen = 0;
uint8_t BLE_DataReady = 0; // 数据接收完成标志，避免中断内操作OLED
uint8_t OLED_wannianliUpdate = 0;
uint8_t ConnState = 0; // 蓝牙连接状态：0-未连接，1-已连接

typedef struct{
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
} CAL_Data;

CAL_Data wannianli = {8,0,0};

int main(void)
{
    SYSCFG_DL_init();
    OLED_Init();
	NVIC_EnableIRQ(UART_BLE_INST_INT_IRQN);
	NVIC_EnableIRQ(TIMER2_INST_INT_IRQN);
	DL_TimerA_startCounter(TIMER2_INST);
	
	// 初始化LED8状态为未连接状态（亮起）
	DL_GPIO_clearPins(LED_PIN_8_PORT, LED_PIN_8_PIN);
	
	OLED_Clear();
	OLED_ShowImage(0, 0, 128, 64, Ark);
	OLED_Update();
	delay_ms(500);
	OLED_Clear();
	OLED_Update();
	
    while (1) 
    {
        KeyNum = GetKeyNum();
		if (KeyNum != 0)
		{
			// 将按键值转换为ASCII字符发送给蓝牙模块
			uint8_t key_ascii = '0' + KeyNum;
			if (KeyNum <= 9) {
				// 按键1-9发送对应数字字符
				DL_UART_transmitDataBlocking(UART_BLE_INST, key_ascii);
			} else if (KeyNum <= 16) {
				// 按键10-16发送A-G字符
				DL_UART_transmitDataBlocking(UART_BLE_INST, 'A' + (KeyNum - 10));
			}
			
			// 添加换行符以便于蓝牙模块接收处理
			DL_UART_transmitDataBlocking(UART_BLE_INST, '\r');
			DL_UART_transmitDataBlocking(UART_BLE_INST, '\n');
		}
		
		if (OLED_wannianliUpdate == 1)
		{
			OLED_ShowNum(0,0,wannianli.hour,2,OLED_8X16);
			OLED_ShowNum(24,0,wannianli.min,2,OLED_8X16);
			OLED_ShowNum(48,0,wannianli.sec,2,OLED_8X16);
			OLED_wannianliUpdate = 0;
			OLED_Update();
		}

		if (BLE_DataReady == 1)
        {
            wannianli.hour = BLE_RecBuf[0]*10 + BLE_RecBuf[1];
			wannianli.min = BLE_RecBuf[2]*10 + BLE_RecBuf[3];
			wannianli.sec = BLE_RecBuf[4]*10 + BLE_RecBuf[5]; //比较反直觉的是，因为屏幕是一秒一刷新，传入12 30 12，屏幕会在下一秒更新，出现12 30 13
			
            OLED_Update(); // 刷新屏幕
			BLE_DataReady = 0;
        }
		OLED_Update();
	}
}

//数据包的结构为[year,month,day,hour,min,sec][2024,11,23,13,45,12]
//现在的算法可以读取16位有效数字，为了测试和简便考虑，在此只读取六位作为时分秒
void UART_BLE_INST_IRQHandler()
{
	// 必须读取接收数据，清空RX FIFO
	uint8_t rx_data = DL_UART_Main_receiveData(UART_BLE_INST);

	// ========== 解析时间数据包逻辑 ==========
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
        BLE_DataReady = 1; // 标记数据就绪，主循环处理显示
	}
	else if (BLE_RecFlag == 1)
	{
		if (rx_data >= '0' && rx_data <= '9')
		{
			BLE_RecBuf[BLE_RecPos] = rx_data - '0';
			BLE_RecPos++;
			if (BLE_RecPos >= 16)
			{
				BLE_RecFlag = 0;BLE_RecPos = 0;BLE_DataReady = 0;
				memset(BLE_RecBuf, 0, sizeof(BLE_RecBuf));Datalen = 0;
			}
		}
		else if (rx_data == ',')
		{
			// 忽略逗号，不做操作
		}
		else
		{
			BLE_RecFlag = 0;BLE_RecPos = 0;Datalen = 0;BLE_DataReady = 0;
			memset(BLE_RecBuf, 0, sizeof(BLE_RecBuf));
		}
	}
	DL_GPIO_togglePins(LED_PIN_1_PORT, LED_PIN_1_PIN);
}

void TIMER2_INST_IRQHandler()
{
	wannianli.sec += 1;
	
	// 每秒发送"hello world"给蓝牙模块
	const char* hello_msg = "hello world\r\n";
	for (int i = 0; hello_msg[i] != '\0'; i++) {
		DL_UART_Main_transmitDataBlocking(UART_BLE_INST, hello_msg[i]);
	}
	
	OLED_wannianliUpdate = 1;
	if (wannianli.sec >= 60)
	{
		wannianli.sec = 0;
		wannianli.min += 1;
		if (wannianli.min >= 60)
		{
			wannianli.min = 0;
			wannianli.hour += 1;
			if (wannianli.hour>=24){wannianli.hour=0;}
		}
	}
}