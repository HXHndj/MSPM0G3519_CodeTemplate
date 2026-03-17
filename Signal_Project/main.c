#include "ti_msp_dl_config.h"
#include "bsp.h"
#include "delay.h"

uint8_t KeyNum = 0;

volatile uint16_t adc_buffer[2048]; // DMA数据转运
volatile bool g_adc_data_ready = false;
WaveResult_t g_WaveResult;

bool g_Measure_mode = 0;

int main(void)
{
	/*  系统初始化配置  */
    SYSCFG_DL_init();
    OLED_Init();

	/*  系统外设启动  */
	// 按键所需timer及中断
	NVIC_EnableIRQ(TIMER_KEY_INST_INT_IRQN);
	NVIC_EnableIRQ(TIMER_ADC_INST_INT_IRQN);
	NVIC_EnableIRQ(DMA_INT_IRQn);

	DL_TimerG_startCounter(TIMER_KEY_INST);
	
	OLED_Clear();
	OLED_ShowString(0,0,"Signal Measurer",OLED_8X16);
	OLED_ShowString(0,16,"Press 1 Single",OLED_6X8);
	OLED_ShowString(0,24,"Press 2 Update",OLED_6X8);
	OLED_ShowString(0,32,"Press 5 Seque On",OLED_6X8);
	OLED_ShowString(0,40,"Press 6 Seque Off",OLED_6X8);
	OLED_Update();
	delay_ms(500);
	
    while (1) 
    {
		KeyNum = GetKeyNum_NonBlocking();
		if(KeyNum != 0)
		{
			switch (KeyNum)
			{
				case 1:
					OLED_Clear();
					OLED_ShowString(0,0,"Measureing",OLED_8X16);
					OLED_Update();
				
					g_adc_data_ready = false;
					DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID, DL_ADC12_getMemResultAddress(ADC12_0_INST, DL_ADC12_MEM_IDX_0));
					DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&adc_buffer[0]);
					DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, 2048);	
					DL_DMA_enableChannel(DMA,DMA_CH0_CHAN_ID);// 启动 DMA 通道
				
					DL_TimerA_startCounter(TIMER_ADC_INST);// 启动定时器（开始发送 100kHz 的脉冲，触发 ADC）

					while (g_adc_data_ready == false) {}
					DL_TimerA_stopCounter(TIMER_ADC_INST);
					Process_Waveform(adc_buffer, 2048, &g_WaveResult);
					
					OLED_Clear();
					OLED_ShowString(0,0,"Done!!",OLED_8X16);
					OLED_ShowString(0,16,"Press 2 To Update",OLED_6X8);
				break;
						
				case 2:		
					OLED_Clear();
					// 显示 Vpp 和 频率 (限制小数位数)
					OLED_ShowString(0,0,"Vpp:",OLED_6X8);
					OLED_ShowFloatNum(24, 0, g_WaveResult.Vpp, 1,3,OLED_6X8);
					OLED_ShowString(60, 0, "V", OLED_6X8);
				
					OLED_ShowString(0,8,"Freq:",OLED_6X8);
					OLED_ShowFloatNum(30, 8, g_WaveResult.Freq, 5,1,OLED_6X8);
					OLED_ShowString(78,8,"Hz",OLED_6X8);
						
					OLED_ShowString(0,16,"Vrms:",OLED_6X8);
					OLED_ShowFloatNum(30, 16, g_WaveResult.Vrms,  1,3,OLED_6X8);
					OLED_ShowString(60, 16, "V", OLED_6X8);
					
					OLED_ShowString(0,24,"Vdc:",OLED_6X8);
					OLED_ShowFloatNum(30, 24, g_WaveResult.Vdc,  1,3,OLED_6X8);
					OLED_ShowString(60, 24, "V", OLED_6X8);
				
					// 根据波形类型显示字符串
					if(g_WaveResult.Type == WAVE_SINE)        OLED_ShowString(0, 48, "Sine", OLED_6X8);
					else if(g_WaveResult.Type == WAVE_SQUARE) 
					{
						OLED_ShowString(0, 48, "Square", OLED_6X8);
						OLED_ShowFloatNum(64, 48, g_WaveResult.Duty, 2,2,OLED_6X8); // 显示占空比
						OLED_ShowString(100, 48, "%", OLED_6X8);
					}
					else if(g_WaveResult.Type == WAVE_TRIANGLE) OLED_ShowString(0, 48, "Triangle", OLED_6X8);
					else if(g_WaveResult.Type == WAVE_DC) OLED_ShowString(0, 48, "DC", OLED_6X8);
					
				break;
					
				case 3:g_Measure_mode = 1;break;
				case 4:g_Measure_mode = 0;break;
					
				default :break;
			}
		}
		
		if (g_Measure_mode == 1)
		{
			OLED_ShowString(0,32,"On ",OLED_6X8);
			
			g_adc_data_ready = false;
			DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID, DL_ADC12_getMemResultAddress(ADC12_0_INST, DL_ADC12_MEM_IDX_0));
			DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&adc_buffer[0]);
			DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, 2048);	
			DL_DMA_enableChannel(DMA,DMA_CH0_CHAN_ID);// 启动 DMA 通道
		
			DL_TimerA_startCounter(TIMER_ADC_INST);// 启动定时器（开始发送 100kHz 的脉冲，触发 ADC）

			while (g_adc_data_ready == false) {}
				
			if (g_Measure_mode)
			{
				DL_TimerA_stopCounter(TIMER_ADC_INST);
				Process_Waveform(adc_buffer, 2048, &g_WaveResult);
				
				OLED_Clear();
				// 显示 Vpp 和 频率 (限制小数位数)
				OLED_ShowString(0,0,"Vpp:",OLED_6X8);
				OLED_ShowFloatNum(24, 0, g_WaveResult.Vpp, 1,3,OLED_6X8);
				OLED_ShowString(60, 0, "V", OLED_6X8);
			
				OLED_ShowString(0,8,"Freq:",OLED_6X8);
				OLED_ShowFloatNum(30, 8, g_WaveResult.Freq, 5,1,OLED_6X8);
				OLED_ShowString(78,8,"Hz",OLED_6X8);
					
				OLED_ShowString(0,16,"Vrms:",OLED_6X8);
				OLED_ShowFloatNum(30, 16, g_WaveResult.Vrms,  1,3,OLED_6X8);
				OLED_ShowString(60, 16, "V", OLED_6X8);
			
				OLED_ShowString(0,24,"Vdc:",OLED_6X8);
				OLED_ShowFloatNum(30, 24, g_WaveResult.Vdc,  1,3,OLED_6X8);
				OLED_ShowString(60, 24, "V", OLED_6X8);
			
			
				// 根据波形类型显示字符串
				if(g_WaveResult.Type == WAVE_SINE)        OLED_ShowString(0, 48, "Sine", OLED_6X8);
				else if(g_WaveResult.Type == WAVE_SQUARE) 
				{
					OLED_ShowString(0, 48, "Square", OLED_6X8);
					OLED_ShowFloatNum(64, 48, g_WaveResult.Duty, 2,2,OLED_6X8); // 显示占空比
					OLED_ShowString(100, 48, "%", OLED_6X8);
				}
				else if(g_WaveResult.Type == WAVE_TRIANGLE) OLED_ShowString(0, 48, "Triangle", OLED_6X8);
				else if(g_WaveResult.Type == WAVE_DC) OLED_ShowString(0, 48, "DC", OLED_6X8);
			}
		}
		
		else
		{
			OLED_ShowString(0,32,"Off",OLED_6X8);
		}
			OLED_Update();
			delay_ms(50);
	}
}

void DMA_IRQHandler(void) 
{
	uint32_t pending_irq = DL_DMA_getPendingInterrupt(DMA);
	
    if (pending_irq == DL_DMA_EVENT_IIDX_DMACH0)
	{
        g_adc_data_ready = true; // 标记数据准备就绪
    }
}
