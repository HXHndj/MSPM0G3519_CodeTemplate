#include "ti_msp_dl_config.h"
#include "bsp.h"
#include "KeyBroad.h"
#include "OLED.h"
#include "delay.h"
#include "ZW101.h"

/*
	引脚分配（按模块从左往右的顺序）
	V_Touch  -- VCC 			触摸检测，常态供电
	Touch_Out  -- PB0 			若有触摸，则为高电平
	VCC  -- PB1					识别模块的电源，可在检测到触摸后再供电
	TX 	-- TX					接单片机RX
	RX 	-- TX					接单片机TX
	GND  -- GND 				与单片机共地
*/

uint8_t KeyNum = 0;
volatile bool is_finger_touched = false;

int main(void)
{
    SYSCFG_DL_init();
    OLED_Init();
    NVIC_EnableIRQ(ZW_INT_IRQN);
    
	DL_GPIO_setPins(GPIOB, ZW_ZW_Vcc_PIN);
    delay_ms(200); // 等待指纹模块内部芯片开机就绪
    ZW101_Init(); // 清理一下刚上电时串口可能产生的乱码毛刺
    
    uint16_t next_enroll_id = 0; 
    if (ZW101_GetFreeID(&next_enroll_id)) {} 
    else 
    {
        next_enroll_id = 1; // 获取失败：可能是通信异常，也可能是库满了(已达50个)
    }

    DL_GPIO_clearPins(GPIOB, ZW_ZW_Vcc_PIN);
    // OLED 显示待机主界面
    OLED_Clear();
    OLED_ShowNum(0, 0,next_enroll_id,2,OLED_8X16); 
    OLED_Update();

    // 获取空闲 ID

    delay_ms(100);

    while (1) 
    {
        KeyNum = GetKeyNum(); 
        
        // ==========================================
        // 操作 1：录入指纹 (假设按下按键 1)
        // ==========================================
        if (KeyNum == 1) 
		{
            // 1. 录入属于主动操作，需要单片机主动给模块上电
            DL_GPIO_setPins(GPIOB, ZW_ZW_Vcc_PIN);
            delay_ms(300); // 等待模块启动
            ZW101_Init();  // 清空串口FIFO
            
            OLED_Clear();
            OLED_ShowString(0, 0, "Enrolling Mode", OLED_8X16);
            OLED_ShowString(0, 16, "Press 10 times", OLED_8X16);
            OLED_Update();
            
            ZW101_LedCtrl(ZW101_LED_BREATH, ZW101_LED_BLUE, 0); // 蓝灯闪烁提示开始录入
            
            // 2. 调用录入函数 
            if (ZW101_Enroll(next_enroll_id,10)) 
			{
                OLED_Clear();
                OLED_ShowString(0, 16, "Enroll Success!", OLED_8X16);
                OLED_Update();
                ZW101_LedCtrl(ZW101_LED_ON, ZW101_LED_GREEN, 0); // 绿灯长亮
                next_enroll_id++; // 成功后，ID号加1，为下次录入做准备
            } 
			else 
			{
                OLED_Clear();
                OLED_ShowString(0, 16, "Enroll Failed!", OLED_8X16);
                OLED_Update();
                ZW101_LedCtrl(ZW101_LED_FLASH, ZW101_LED_RED, 3); // 红灯闪烁报错
            }
            delay_ms(300); // 留出时间给用户看结果
            
            // 3. 恢复低功耗与主界面
            DL_GPIO_clearPins(GPIOB, ZW_ZW_Vcc_PIN); // 断电
            is_finger_touched = false;
			OLED_Clear();
			OLED_ShowNum(0, 0,next_enroll_id,2,OLED_8X16); 
			OLED_Update();
        }
        
        // ==========================================
        // 操作 2：清空指纹库 (假设按下按键 2)
        // ==========================================
        if (KeyNum == 2) 
		{
            // 1. 主动上电
            DL_GPIO_setPins(GPIOB, ZW_ZW_Vcc_PIN);
            delay_ms(300); 
            ZW101_Init();
            
            OLED_Clear();
            OLED_ShowString(0, 0, "Clearing Lib...", OLED_8X16);
            OLED_Update();
            
            // 2. 调用清空函数
            if (ZW101_Clear()) 
			{
                OLED_ShowString(0, 32, "Clear Success!", OLED_8X16);
                ZW101_LedCtrl(ZW101_LED_FLASH, ZW101_LED_GREEN, 2);
                next_enroll_id = 0; // 清空库后，录入ID需要重置回1
            } 
			else 
			{
                OLED_ShowString(0, 32, "Clear Failed!", OLED_8X16);
                ZW101_LedCtrl(ZW101_LED_FLASH, ZW101_LED_RED, 2);
            }
			
            OLED_Update();
            delay_ms(300);
            
            // 3. 恢复低功耗与主界面
            DL_GPIO_clearPins(GPIOB, ZW_ZW_Vcc_PIN);
            is_finger_touched = false;
			OLED_Clear();
			OLED_ShowNum(0, 0,next_enroll_id,2,OLED_8X16); 
			OLED_Update();
        }

        // ==========================================
        // 操作 3：检验指纹 (你的中断唤醒逻辑)
        // ==========================================
        if (is_finger_touched) 
		{
			// 1. 模块已经上电（在中断中通过 ZW_ZW_Vcc_PIN 完成了 [cite: 1564]）
			// 增加一个小延时等待模块初始化完成（规格书建议启动时间 < 0.1s） 
			delay_ms(100); 
			ZW101_Init(); 
			
			uint16_t mid = 0;
			// 2. 调用带有抬起保护的识别函数
			// 该函数会一直阻塞直到用户拿开手指
			uint8_t res = ZW101_Identify(&mid);
			
			if (res == 1) 
			{
				// 识别成功：开锁逻辑
				OLED_Clear();
				OLED_ShowString(0, 16, "Access Granted", OLED_8X16);
				OLED_Update();
				// 此处可以控制电机或电磁锁
			}
			else if (res == 2)
			{
				// 识别失败处理
				OLED_Clear();
				OLED_ShowString(0, 16, "Access Denied", OLED_8X16);
				OLED_Update();
			}
			
			// 3. 只有手指抬起后才会执行到这里，彻底断电进入低功耗
			delay_ms(200); // 留出一点时间让用户看清 OLED 结果
			DL_GPIO_clearPins(GPIOB, ZW_ZW_Vcc_PIN); // 断开 VCC 节省静态功耗
			is_finger_touched = false; // 重置中断标志位
			
			OLED_Clear();
			OLED_ShowNum(0, 0,next_enroll_id,2,OLED_8X16); 
			OLED_Update();
		}
    }
}

void GROUP1_IRQHandler(void)
{
    if (DL_Interrupt_getPendingGroup(DL_INTERRUPT_GROUP_1) == ZW_INT_IIDX) 
    {
        if (DL_GPIO_getEnabledInterruptStatus(GPIOB, ZW_TouchOut_PIN)) 
        {
            DL_GPIO_clearInterruptStatus(GPIOB, ZW_TouchOut_PIN);
            DL_GPIO_setPins(GPIOB, ZW_ZW_Vcc_PIN);               
            is_finger_touched = true;                          
        }
    }
}