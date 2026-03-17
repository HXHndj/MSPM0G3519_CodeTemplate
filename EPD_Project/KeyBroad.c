#include "ti_msp_dl_config.h"
#include "KeyBroad.h"
#include "delay.h"

static volatile uint8_t g_LastKey = 0;   
static volatile uint8_t g_FinalKey = 0;
static volatile uint16_t g_HoldCount = 0; 

// 下面两个宏定义用于调节手感，需要根据实际的定时器中断频率来微调
// 定时器中断是 10ms 触发一次：
#define LONG_PRESS_THRESHOLD 30  // 首次长按触发阈值 (30 * 10ms = 300ms)
#define CONTINUOUS_RATE      5  // 触发后的连发速度 (CONTINUOUS_RATE(5) * 10ms = 50ms / 次)

// 这是一个延时的按键读取函数，仅需要配置GPIO口就可以使用
// 此函数为松手返回键值
// GPIO口的配置，因为是强下拉弱上拉，故
// 需要配置行 H1-4 为输出，悬空；列 V1-4 为输入，接上拉电阻
uint8_t GetKeyNum(void)
{
	uint32_t Hang[4] = {KeyBoard_H1_PIN, KeyBoard_H2_PIN, KeyBoard_H3_PIN, KeyBoard_H4_PIN};
	uint32_t Lie[4]  = {KeyBoard_V1_PIN, KeyBoard_V2_PIN, KeyBoard_V3_PIN, KeyBoard_V4_PIN};
	uint8_t KeyNum = 0;
	for (int i = 0; i < 4; i++)
	{
		DL_GPIO_clearPins(KeyBoard_PORT, Hang[i]);			//1 2 3 4
		DL_GPIO_setPins(KeyBoard_PORT, Hang[(i+1)%4]);		//5 6 7 8
		DL_GPIO_setPins(KeyBoard_PORT, Hang[(i+2)%4]);		//9 10 11 12
		DL_GPIO_setPins(KeyBoard_PORT, Hang[(i+3)%4]);		//13 14 15 16
		
		for (int j = 0; j < 4; j++)
		{
			if (DL_GPIO_readPins(KeyBoard_PORT, Lie[j]) == 0)
			{
				delay_ms(20); // 消抖：等待20ms，确认按键稳定
				if (DL_GPIO_readPins(KeyBoard_PORT, Lie[j]) == 0)// 二次确认按键按下
				{
					KeyNum = i * 4 + j + 1;
					while (DL_GPIO_readPins(KeyBoard_PORT, Lie[j]) == 0);// 等待按键释放，避免重复触发
					return KeyNum;
				}
			}
		}
	}
	for (int k = 0; k < 4; k++) 
	{
        DL_GPIO_setPins(KeyBoard_PORT, Hang[k]);
    }
	return KeyNum;
}

// 此函数不会阻塞主程序，可以用于部分时序要求严格的场景
// 但需要搭配Timer定时器,和 GetKeyNum_NonBlocking(void)使用
uint8_t Matrix_Scan_Raw(void)
{
    static uint32_t Hang[4] = {KeyBoard_H1_PIN, KeyBoard_H2_PIN, KeyBoard_H3_PIN, KeyBoard_H4_PIN};
    static uint32_t Lie[4]  = {KeyBoard_V1_PIN, KeyBoard_V2_PIN, KeyBoard_V3_PIN, KeyBoard_V4_PIN};
    DL_GPIO_setPins(KeyBoard_PORT, (KeyBoard_H1_PIN | KeyBoard_H2_PIN | KeyBoard_H3_PIN | KeyBoard_H4_PIN));
    for (int i = 0; i < 4; i++)
    {
        DL_GPIO_clearPins(KeyBoard_PORT, Hang[i]);
        
		__asm("nop"); __asm("nop"); __asm("nop");// 稍微给一点点 IO 电平稳定的时间
        
		uint32_t col_pins = DL_GPIO_readPins(KeyBoard_PORT, Lie[0] | Lie[1] | Lie[2] | Lie[3]);
        
        // 如果这一行有任何按键按下，才进入细分判断，否则直接跳到下一行
        if (col_pins != (Lie[0] | Lie[1] | Lie[2] | Lie[3])) 
        {
            for (int j = 0; j < 4; j++)
            {
                if (!(col_pins & Lie[j])) // 判断哪一列被拉低了
                {
                    DL_GPIO_setPins(KeyBoard_PORT, Hang[i]);// 扫描结束后，在返回前恢复行电平，防止影响下次扫描
                    return (i * 4 + j + 1); 
                }
            }
        }
		DL_GPIO_setPins(KeyBoard_PORT, Hang[i]);
    }
    return 0; // 扫完一轮发现没按键
}

void TIMER_KEY_INST_IRQHandler(void)
{
	uint8_t current_key = Matrix_Scan_Raw(); //
	
    switch (DL_TimerA_getPendingInterrupt(TIMA0)) // 检查中断标志位
	{
        case DL_TIMER_IIDX_ZERO: // 定时器溢出/到达零点中断
		{
            if (current_key != 0) // 有按键按下
            {
                if (g_LastKey == 0) 
                {
                    // 1. 按键刚按下 (首次触发)
                    g_FinalKey = current_key; // 产生一个有效键值
                    g_HoldCount = 0;          // 清零长按计时器
                }
                else if (current_key == g_LastKey) 
                {
                    // 2. 保持按下同一个键
                    g_HoldCount++;
                    
                    if (g_HoldCount >= LONG_PRESS_THRESHOLD) // 达到长按判定时间
                    {
                        g_FinalKey = current_key; // 再次产生一个有效键值（触发连发）
                        
                        // 扣除连发周期，这样下一次累加到阈值只需要 CONTINUOUS_RATE 的时间
                        g_HoldCount -= CONTINUOUS_RATE; 
                    }
                }
            }
            else 
            {
                // 3. 按键松开
                g_HoldCount = 0; // 松开按键清零计时器
            }
			
			g_LastKey = current_key; // 更新状态
            break;
		}
        default:break; //
    }
}

// 此函数为按下就返回键值
uint8_t GetKeyNum_NonBlocking(void)
{
	uint8_t temp;
    
    // 临界区保护：防止清除 g_FinalKey 时被中断打断导致丢键
	DL_TimerA_disableInterrupt(TIMER_KEY_INST, DL_TIMERA_INTERRUPT_ZERO_EVENT); // 关按键定时器
    temp = g_FinalKey;
    g_FinalKey = 0; 
    DL_TimerA_enableInterrupt(TIMER_KEY_INST, DL_TIMERA_INTERRUPT_ZERO_EVENT); // 关按键定时器
    
    return temp;
}
