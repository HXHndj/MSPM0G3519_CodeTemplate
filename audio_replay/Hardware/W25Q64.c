#include "W25Q64.h"

/* ================== 硬件 SPI 底层配置层 ================== */
/*
 * 说明：由于使用了 MSPM0 的 SysConfig，外设初始化已在 
 * SYSCFG_DL_init() 中完成，此处无需再次初始化引脚和 SPI 寄存器。
 */

/*
	在此对W25Q64的相关构造进行一个说明，具体内容也可以查看江协科技的教学视频
	
	首先，这款Flash的存储有8MB，即8*1024KB，即8M个uint8_t的数据
	这些内存按如下规则划分：
	8MB内存被分为128个Block，即128个块，每个块的内存为64KB
	每个块又被分为16个Sector，即16个扇区，每个扇区的内存为4KB，扇区是擦除的最小单位
	进行写操作的最小单位是Page，因为芯片内部的RAM只有256B，即一页为256B
	
	对判忙函数的说明：
	此段代码中，为死循环，同时还提供两种思路来拓展：
	第一，可以利用TimeOut跳出死循环，再对卡死进行处理，防止主程序等待过久
	第二，可以利用读取寄存器，再进行其他操作前，自动判断是否处于忙状态
	不过，除了整片擦除的操作外，其他操作一般耗时较小，故我们使用TimeOut就可以满足一般需求
*/

/**
  * 函    数：SPI起始
  * 参    数：无
  * 返 回 值：无
  */
void W25Q64_SPI_Start(void)
{
    DL_GPIO_clearPins(W25Q64_PORT, W25Q64_W25Q64_CS_PIN); // 拉低 CS 片选信号，开始时序
}

/**
  * 函    数：SPI终止
  * 参    数：无
  * 返 回 值：无
  */
void W25Q64_SPI_Stop(void)
{
    DL_GPIO_setPins(W25Q64_PORT, W25Q64_W25Q64_CS_PIN); // 拉高 CS 片选信号，终止时序
}

/**
  * 函    数：SPI交换传输一个字节，使用硬件 SPI
  * 参    数：ByteSend 要发送的一个字节
  * 返 回 值：接收的一个字节
  */
uint8_t W25Q64_SPI_SwapByte(uint8_t ByteSend)
{
    DL_SPI_transmitData8(SPI_W25Q64_INST, ByteSend);
    
    while(DL_SPI_isRXFIFOEmpty(SPI_W25Q64_INST))
    {
        // 阻塞等待硬件发送接收完成
    }
    
    return DL_SPI_receiveData8(SPI_W25Q64_INST);
}

/**
  * 函    数：W25Q64初始化
  * 参    数：无
  * 返 回 值：无
  */
void W25Q64_Init(void)
{
    // 硬件配置已经在 main.c 的 SYSCFG_DL_init() 完成
    // 此处只需要确保上电时片选处于拉高（空闲）状态即可
    W25Q64_SPI_Stop();
}


/* ================== 协议层  ================== */

/**
  * 函    数：W25Q64读取ID号
  * 参    数：MID 工厂ID，使用输出参数的形式返回
  * 参    数：DID 设备ID，使用输出参数的形式返回
  * 返 回 值：无
  */
void W25Q64_ReadID(uint8_t *MID, uint16_t *DID)
{
	W25Q64_SPI_Start();										//SPI起始
	W25Q64_SPI_SwapByte(W25Q64_JEDEC_ID);					//交换发送读取ID的指令
	*MID = W25Q64_SPI_SwapByte(W25Q64_DUMMY_BYTE);			//交换接收MID，通过输出参数返回
	*DID = W25Q64_SPI_SwapByte(W25Q64_DUMMY_BYTE);			//交换接收DID高8位
	*DID <<= 8;												//高8位移到高位
	*DID |= W25Q64_SPI_SwapByte(W25Q64_DUMMY_BYTE);			//或上交换接收DID的低8位，通过输出参数返回
	W25Q64_SPI_Stop();										//SPI终止
}

/**
  * 函    数：W25Q64写使能
  * 参    数：无
  * 返 回 值：无
  */
void W25Q64_WriteEnable(void)
{
	W25Q64_SPI_Start();										//SPI起始
	W25Q64_SPI_SwapByte(W25Q64_WRITE_ENABLE);				//交换发送写使能的指令
	W25Q64_SPI_Stop();										//SPI终止
}

/**
  * 函    数：W25Q64等待忙
  * 参    数：无
  * 返 回 值：无
  */
void W25Q64_WaitBusy(void)
{
//	uint32_t Timeout;
	W25Q64_SPI_Start();										//SPI起始
	W25Q64_SPI_SwapByte(W25Q64_READ_STATUS_REGISTER_1);		//交换发送读状态寄存器1的指令
//	Timeout = 100000;										//给定超时计数时间
	while ((W25Q64_SPI_SwapByte(W25Q64_DUMMY_BYTE) & 0x01) == 0x01)	//循环等待忙标志位
	{
//		Timeout --;											//等待时，计数值自减
//		if (Timeout == 0)									//自减到0后，等待超时
//		{
//			/*超时的错误处理代码，可以添加到此处*/
//			break;											//跳出等待，不等了
//		}
	}
	W25Q64_SPI_Stop();										//SPI终止
}

/**
  * 函    数：W25Q64读取数据
  * 参    数：Address 读取数据的起始地址，范围：0x000000~0x7FFFFF
  * 参    数：DataArray 用于接收读取数据的数组，通过输出参数返回
  * 参    数：Count 要读取数据的数量，范围：0~0x800000
  * 返 回 值：无
  */
void W25Q64_ReadData(uint32_t Address, uint8_t *DataArray, uint32_t Count)
{
	uint32_t i;
	W25Q64_SPI_Start();								//SPI起始
	W25Q64_SPI_SwapByte(W25Q64_READ_DATA);			//交换发送读取数据的指令
	W25Q64_SPI_SwapByte(Address >> 16);				//交换发送地址23~16位
	W25Q64_SPI_SwapByte(Address >> 8);				//交换发送地址15~8位
	W25Q64_SPI_SwapByte(Address);					//交换发送地址7~0位
	for (i = 0; i < Count; i ++)					//循环Count次
	{
		DataArray[i] = W25Q64_SPI_SwapByte(W25Q64_DUMMY_BYTE);	//依次在起始地址后读取数据
	}
	W25Q64_SPI_Stop();								//SPI终止
}

/**
  * 函    数：W25Q64页编程
  * 参    数：Address 页编程的起始地址，范围：0x000000~0x7FFFFF
  * 参    数：DataArray	用于写入数据的数组
  * 参    数：Count 要写入数据的数量，范围：0~256
  * 返 回 值：无
  * 注意事项：写入的地址范围不能跨页,如果出现跨页，则会覆盖内存起始位置的数据，类似于环状结构
  */
void W25Q64_PageProgram(uint32_t Address, uint8_t *DataArray, uint16_t Count)
{
	uint16_t i;
	
	W25Q64_WriteEnable();							//写使能
	
	W25Q64_SPI_Start();								//SPI起始
	W25Q64_SPI_SwapByte(W25Q64_PAGE_PROGRAM);		//交换发送页编程的指令
	W25Q64_SPI_SwapByte(Address >> 16);				//交换发送地址23~16位
	W25Q64_SPI_SwapByte(Address >> 8);				//交换发送地址15~8位
	W25Q64_SPI_SwapByte(Address);					//交换发送地址7~0位
	for (i = 0; i < Count; i ++)					//循环Count次
	{
		W25Q64_SPI_SwapByte(DataArray[i]);			//依次在起始地址后写入数据
	}
	W25Q64_SPI_Stop();								//SPI终止
	W25Q64_WaitBusy();								//等待忙
}

/**
  * 函    数：W25Q64 跨页连续写数据 (最高级写入函数)
  * 参    数：Address 起始地址，范围：0x000000~0x7FFFFF
  * 参    数：DataArray	用于写入数据的数组
  * 参    数：Count 要写入数据的数量，范围不限
  * 返 回 值：无
  * 说    明：自动处理256字节页对齐问题，防止数据环绕覆盖，调用这个函数之前要先对内存进行擦除
  */
void W25Q64_BufferWrite(uint32_t Address, uint8_t *DataArray, uint32_t Count)
{
    // 计算当前页还剩下多少个字节可以写
    uint16_t PageRemain = 256 - (Address % 256);
    
    // 如果要写入的数据量小于等于当前页剩余空间，直接一次写完
    if (Count <= PageRemain) 
    {
        PageRemain = Count;
    }
    
    while (1)
    {
        // 调用底层单页编程函数
        W25Q64_PageProgram(Address, DataArray, PageRemain);
        
        // 如果数据全写完了，退出循环
        if (Count == PageRemain) 
        {
            break;
        }
        else // 还没写完，准备写下一页
        {
            DataArray += PageRemain; // 数据指针偏移
            Address += PageRemain;   // 写入地址偏移
            Count -= PageRemain;     // 剩余需要写入的数量递减
            
            // 如果剩余数据还是大于256，那下一页可以直接写满256；否则把剩下的写完
            PageRemain = (Count > 256) ? 256 : Count;
        }
    }
}

/**
  * 函    数：W25Q64扇区擦除（4KB）
  * 参    数：Address 指定扇区的地址，范围：0x000000~0x7FFFFF
  * 返 回 值：无
  */
void W25Q64_SectorErase(uint32_t Address)
{
	W25Q64_WriteEnable();						//写使能
	
	W25Q64_SPI_Start();								//SPI起始
	W25Q64_SPI_SwapByte(W25Q64_SECTOR_ERASE_4KB);	//交换发送扇区擦除的指令
	W25Q64_SPI_SwapByte(Address >> 16);				//交换发送地址23~16位
	W25Q64_SPI_SwapByte(Address >> 8);				//交换发送地址15~8位
	W25Q64_SPI_SwapByte(Address);					//交换发送地址7~0位
	W25Q64_SPI_Stop();								//SPI终止
	
	W25Q64_WaitBusy();							//等待忙
}

/**
  * 函    数：W25Q64 块擦除（64KB）
  * 参    数：Address 指定块内的任意地址，范围：0x000000~0x7FFFFF
  * 返 回 值：无
  */
void W25Q64_BlockErase_64KB(uint32_t Address)
{
    W25Q64_WriteEnable();						// 写使能
    
    W25Q64_SPI_Start();								// SPI起始
    W25Q64_SPI_SwapByte(W25Q64_BLOCK_ERASE_64KB);	// 交换发送64KB块擦除的指令 (W25Q64_BLOCK_ERASE_64KB)
    W25Q64_SPI_SwapByte(Address >> 16);				// 交换发送地址23~16位
    W25Q64_SPI_SwapByte(Address >> 8);				// 交换发送地址15~8位
    W25Q64_SPI_SwapByte(Address);					// 交换发送地址7~0位
    W25Q64_SPI_Stop();								// SPI终止
    
    W25Q64_WaitBusy();							// 等待忙
}

/**
  * 函    数：W25Q64 整片擦除
  * 参    数：无
  * 返 回 值：无
  * 注    意：耗时较长（可能需要几十秒）
  */
void W25Q64_ChipErase(void)
{
    W25Q64_WriteEnable();							// 写使能
    
    W25Q64_SPI_Start();								// SPI起始
    W25Q64_SPI_SwapByte(W25Q64_CHIP_ERASE);			// 交换发送整片擦除指令 (W25Q64_CHIP_ERASE)
    W25Q64_SPI_Stop();								// SPI终止
    
    W25Q64_WaitBusy();								// 等待擦除完成
}