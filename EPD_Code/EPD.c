#include "EPD.h"
#include "delay.h"

/*******************************************************************
    函数说明:判忙函数
    入口参数:无
    说明:忙状态为1    
*******************************************************************/
void EPD_READBUSY(void) {
    uint32_t timeout = 5000000; // 根据主频调整超时常数
    while(timeout--) {
        if(((DL_GPIO_readPins(EPD_BUSY_PORT, EPD_BUSY_PIN) & EPD_BUSY_PIN) ? 1 : 0) == 0) {
            return; // 正常退出
        }
    }
}

void EPD_GPIOInit(void)
{
    // SPI和GPIO已在SysConfig中初始化，这里确保CS和DC初始状态
    EPD_CS_Set();
    EPD_DC_Set();
    EPD_RES_Set();
}

/**
 * @brief 硬件SPI发送8位数据（基础函数）
 * @param dat 要发送的8位数据
 */
void EPD_WR_Bus(uint8_t dat)
{
    EPD_CS_Clr(); // 拉低片选，选中设备
    
    // 发送数据
    DL_SPI_transmitData8(SPI_0_INST, dat);
    
    // 等待发送完成
    while (DL_SPI_isBusy(SPI_0_INST));
    
    EPD_CS_Set(); // 拉高片选，释放设备
}

/**
 * @brief 写命令寄存器
 * @param reg 寄存器地址
 */
void EPD_WR_REG(uint8_t reg)
{
    EPD_DC_Clr(); // DC=0 表示命令
    EPD_WR_Bus(reg);
    EPD_DC_Set(); // 恢复DC为高（默认数据状态，减少下次切换）
}

/**
 * @brief 写8位数据
 * @param dat 数据
 */
void EPD_WR_DATA8(uint8_t dat)
{
    EPD_DC_Set(); // DC=1 表示数据
    EPD_WR_Bus(dat);
	EPD_DC_Set();
}

/*******************************************************************
    函数说明:硬件复位函数
    入口参数:无
    说明:在E-Paper进入Deepsleep状态后需要硬件复位  
*******************************************************************/
void EPD_HW_RESET(void)
{
	delay_ms(100);
	EPD_RES_Clr();
	delay_ms(10);
	EPD_RES_Set();
	delay_ms(10);
	EPD_READBUSY();
}

/*******************************************************************
    函数说明:更新函数
    入口参数:无  
    说明:更新显示内容到E-Paper    
*******************************************************************/
void EPD_Update(void)
{
	EPD_WR_REG(0x20);
	EPD_READBUSY();
}

/*******************************************************************
    函数说明:休眠函数
    入口参数:无
    说明:屏幕进入低功耗模式    
*******************************************************************/
void EPD_DeepSleep(void)
{
	EPD_WR_REG(0x10);
	EPD_WR_DATA8(0x01);
	delay_ms(200);
}

/*******************************************************************
    函数说明:初始化函数
    入口参数:无
    说明:调整E-Paper默认显示方向
*******************************************************************/
void EPD_Init(void)
{
	EPD_HW_RESET();
	EPD_READBUSY();   
	EPD_WR_REG(0x12);  //SWRESET
	EPD_READBUSY();   
}

/*******************************************************************
    函数说明:清屏函数
    入口参数:无
    说明:E-Paper刷白屏
*******************************************************************/
void EPD_Display_Clear(void)
{
	uint16_t i;
	EPD_WR_REG(0x24);
	for(i=0;i<2888;i++)
	{
		EPD_WR_DATA8(0xFF);
	}  
	EPD_READBUSY();
	EPD_WR_REG(0x26);
	for(i=0;i<2888;i++)
	{
		EPD_WR_DATA8(0x00);
	}  
}

/*******************************************************************
    函数说明:数组数据更新到E-Paper
    入口参数:无
    说明:
*******************************************************************/
void EPD_Display(const uint8_t *imageBW, const uint8_t *imageR)
{
  uint16_t i, j, Width, Height;
  Width = (EPD_W % 8 == 0) ? (EPD_W / 8) : (EPD_W / 8 + 1);
  Height = EPD_H;

  // 发送黑白数据层 (寄存器 0x24)
  EPD_WR_REG(0x24);
  for (j = 0; j < Height; j++) 
  {
    for (i = 0; i < Width; i++) 
    {
      uint32_t addr = i + j * Width;
      /**
       * 核心逻辑：红色优先
       * 在你的缓冲区中，0 代表有色(BLACK/RED)，1 代表白色(WHITE)
       * 如果 imageR[addr] 的某位是 0 (表示该点要画红色)，
       * 我们强制让 imageBW[addr] 的对应位变为 1 (变为白色)。
       * 运算式：BW_输出 = BW_原始 | (~Red_原始)
       */
      uint8_t processedBW = imageBW[addr] | (~imageR[addr]); 
      EPD_WR_DATA8(processedBW);
    }
  }

  // 发送红色数据层 (寄存器 0x26)
  EPD_WR_REG(0x26);
  for (j = 0; j < Height; j++) 
  {
    for (i = 0; i < Width; i++) 
    {
      // 红色层维持原逻辑：取反后发送
      EPD_WR_DATA8(~imageR[i + j * Width]);
    }
  }
}
