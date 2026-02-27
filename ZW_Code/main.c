#include "ti_msp_dl_config.h"
#include "bsp.h"
#include "KeyBroad.h"
#include "OLED.h"
#include "delay.h"
#include "ZW101.h"

/*
	引脚分配（按模块从左往右的顺序）
	V_Touch  -- VCC 			触摸检测，常态供电
	Touch_Out  -- PB0 			若有触摸，则为高电平，接单片机GPIO中断
	VCC  -- PB1					识别模块的电源，可在检测到触摸后再供电
	TX 	-- PB5 (RX)				接单片机RX
	RX 	-- PB4 (TX)				接单片机TX
	GND  -- GND 				与单片机共地
	
	该代码既可以适用于单片机直连指纹模块，若采用模块手册内的低功耗电路
	可以采用以下引脚配对：
	V_Touch  -- VCC 			触摸检测，常态供电
	Touch_Out  -- PB0 			若有触摸，则为高电平，接单片机GPIO中断
	VCC  -- VOUT				识别模块的电源，接到低功耗电路的Vout
	TX 	-- PB5 (RX)				接单片机RX
	RX 	-- PB4 (TX)				接单片机TX
	GND  -- GND 				与单片机共地
	
	低功耗电路：
	CRTL  --  PB1				用于开关驱动电路，不直接给模块供电
	VCC  --  VCC
	GND  -- GND
	VOUT  -- 指纹模块VCC
	
	同时，为了防止单片机Tx漏电，导致低功耗电路的Vout引脚处于2.16V电压状态
	（即PB0，本应处于关闭状态，但Vout电压仍为2.16V的情况）
	在PB0开关驱动电路时，手动添加了软件清零，这样在PB0输出为0时，Vout也为0，不存在漏电现象了
*/

uint8_t KeyNum = 0;
volatile bool is_finger_touched = false;

/**
 * @brief 给指纹模块上电，并恢复串口引脚的复用功能
 */
void ZW101_PowerOn(void)
{
    // 1. 恢复模块 VCC 供电
    DL_GPIO_setPins(GPIOB, ZW_ZW_Vcc_PIN);
    
    // 2. 恢复引脚 UART 复用功能
    DL_GPIO_initPeripheralOutputFunction(GPIO_ZW_Uart_IOMUX_TX, GPIO_ZW_Uart_IOMUX_TX_FUNC);
    DL_GPIO_initPeripheralInputFunction(GPIO_ZW_Uart_IOMUX_RX, GPIO_ZW_Uart_IOMUX_RX_FUNC);
    
    // 3. 重新使能串口外设
    DL_UART_Main_enable(ZW_Uart_INST);
}

/**
 * @brief 给指纹模块断电，并将串口引脚设为高阻态以切断漏电路径
 */
void ZW101_PowerOff(void)
{
    // 1. 停止串口外设工作
    DL_UART_Main_disable(ZW_Uart_INST);
    
    // 2. 将引脚切为高阻态（数字输入，无上下拉，防止电流倒灌）
    DL_GPIO_initDigitalInputFeatures(GPIO_ZW_Uart_IOMUX_TX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
         
    DL_GPIO_initDigitalInputFeatures(GPIO_ZW_Uart_IOMUX_RX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
         
    // 3. 物理切断 VCC 供电
    DL_GPIO_clearPins(GPIOB, ZW_ZW_Vcc_PIN);
}

int main(void)
{
    SYSCFG_DL_init();
    OLED_Init();
    NVIC_EnableIRQ(ZW_INT_IRQN);
    
    // 初始化上电
	ZW101_PowerOn();
    delay_ms(200); // 等待指纹模块内部芯片开机就绪
    ZW101_Init(); // 清理一下刚上电时串口可能产生的乱码毛刺
    
    uint16_t next_enroll_id = 0; 
    if (ZW101_GetFreeID(&next_enroll_id)) {} // 获取空余的指纹ID，从0到49
    else 
    {
        next_enroll_id = 1; // 获取失败：可能是通信异常，也可能是库满了(已达50个)
    }

    // 初始化完毕，进入深度低功耗模式
    ZW101_PowerOff(); 
    
    // OLED 显示待机主界面
    OLED_Clear();
    OLED_ShowNum(0, 0,next_enroll_id,2,OLED_8X16); 
    OLED_Update();

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
            ZW101_PowerOn();
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
            ZW101_PowerOff(); // 断电及防倒灌
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
            ZW101_PowerOn();
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
                next_enroll_id = 0; // 清空库后，录入ID需要重置回0
            } 
			else 
			{
                OLED_ShowString(0, 32, "Clear Failed!", OLED_8X16);
                ZW101_LedCtrl(ZW101_LED_FLASH, ZW101_LED_RED, 2);
            }
			
            OLED_Update();
            delay_ms(300);
            
            // 3. 恢复低功耗与主界面
            ZW101_PowerOff(); // 断电及防倒灌
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
			// 1. 模块已经上电（在中断中通过 ZW101_PowerOn 完成了）
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
			ZW101_PowerOff(); // 彻底断开 VCC 并切断串口寄生电源
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
            // 中断中调用电源恢复函数
            ZW101_PowerOn();               
            is_finger_touched = true;
        }
    }
}
