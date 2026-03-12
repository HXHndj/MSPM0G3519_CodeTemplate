#include "ti_msp_dl_config.h"
#include "bsp.h"

uint8_t KeyNum = 0;
uint8_t MID;							//定义用于存放MID号的变量
uint16_t DID;							//定义用于存放DID号的变量

uint8_t Wdata[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
uint8_t Rdata[17]={0};

int main(void)
{
    SYSCFG_DL_init();
    OLED_Init();
	W25Q64_Init();
	
	OLED_Clear();
	OLED_ShowImage(0, 0, 128, 64, Ark);
	OLED_Update();
	DL_TimerA_startCounter(TIMER_KEY_INST);
	delay_ms(500);
	
		/*显示ID号*/
	W25Q64_ReadID(&MID, &DID);			//获取W25Q64的ID号
	OLED_Clear();
	OLED_ShowHexNum(0, 0, MID, 2,OLED_8X16);		//显示MID
	OLED_ShowHexNum(0, 16, DID, 4,OLED_8X16);		//显示DID
	OLED_Update();
	
	W25Q64_SectorErase(0x000000); // 只擦除第一个 4KB 扇区
	W25Q64_PageProgram(0x000000,Wdata,16);
	W25Q64_ReadData(0x000000,Rdata,17);
	
	OLED_Clear();
	OLED_ShowNum(0,0,Rdata[11],2,OLED_8X16);
	//OLED_ShowHexNum(0,0,Rdata[12],2,OLED_8X16);
	OLED_Update();
    while (1) 
    {
        
	}
}
