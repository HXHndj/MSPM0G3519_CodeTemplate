#include "UartVofa.h"

/*
 * VOFA+ JustFloat 协议发送函数
 * 格式：每个采样点转换为 float32 (4字节) + 帧尾 (0x00 0x00 0x80 0x7F)
 * 单通道模式，每帧 = 1个float + 1个帧尾
 */
void SendAudioToVofaPlus(void)
{
    uint32_t total_samples;
    uint32_t addr;
    uint8_t read_buf[UART_TX_BUF_SIZE];
    uint32_t remaining;
    uint32_t read_size;
    uint32_t samples_in_chunk;
    uint32_t i;
    uint16_t sample_raw;
    float sample_float;
    uint8_t *f_ptr;
    uint8_t percent;
    
    total_samples = current_flash_addr / 2;  // 总采样点数 (uint16_t)
    addr = 0;
    
    OLED_Clear();
    OLED_ShowString(0, 0, "UART Sending...", OLED_8X16);
    OLED_ShowString(0, 16, "0%", OLED_8X16);
    OLED_Update();
    
    while (addr < current_flash_addr)
    {
        remaining = current_flash_addr - addr;
        read_size = (remaining > UART_TX_BUF_SIZE) ? UART_TX_BUF_SIZE : remaining;
        
        // 从W25Q64读取一段录音数据
        W25Q64_ReadData(addr, read_buf, read_size);
        
        // 逐采样点转换并发送
        samples_in_chunk = read_size / 2;
        for (i = 0; i < samples_in_chunk; i++)
        {
            // 读取两个字节组成 uint16_t (小端: W25Q64 低地址存低字节)
            sample_raw = (uint16_t)read_buf[i * 2] | ((uint16_t)read_buf[i * 2 + 1] << 8);
            
            // 将 ADC 原始值转为 float
            sample_float = (float)sample_raw;
            
            // 发送 float32 (4字节, 小端)
            f_ptr = (uint8_t *)&sample_float;
            DL_UART_transmitDataBlocking(UART_0_INST, f_ptr[0]);
            DL_UART_transmitDataBlocking(UART_0_INST, f_ptr[1]);
            DL_UART_transmitDataBlocking(UART_0_INST, f_ptr[2]);
            DL_UART_transmitDataBlocking(UART_0_INST, f_ptr[3]);
            
            // 发送 VOFA+ RawData 帧尾
            DL_UART_transmitDataBlocking(UART_0_INST, VOFA_FRAME_FOOTER0);
            DL_UART_transmitDataBlocking(UART_0_INST, VOFA_FRAME_FOOTER1);
            DL_UART_transmitDataBlocking(UART_0_INST, VOFA_FRAME_FOOTER2);
            DL_UART_transmitDataBlocking(UART_0_INST, VOFA_FRAME_FOOTER3);
        }
        
        addr += read_size;
        
        // 更新进度显示
        percent = (uint8_t)((addr * 100UL) / current_flash_addr);
        OLED_ShowString(0, 16, "   ", OLED_8X16);  // 清除旧数字
        OLED_ShowNum(0, 16, percent, 3, OLED_8X16);
        OLED_ShowString(24, 16, "%", OLED_8X16);
        OLED_Update();
    }
    
    // 发送完成
    OLED_Clear();
    OLED_ShowString(0, 0, "UART Done!", OLED_8X16);
    OLED_ShowNum(0, 16, total_samples, 6, OLED_8X16);
    OLED_ShowString(48, 16, "smp", OLED_8X16);
    OLED_Update();
}

/*
 * VOFA+ JustFloat 协议发送函数（电压值版）
 * 将 ADC 原始值换算为电压 (Vref=3.3V, 12bit) 后发送
 * 格式：每个采样点转换为 float32 (4字节) + 帧尾 (0x00 0x00 0x80 0x7F)
 * 单通道模式，每帧 = 1个float + 1个帧尾
 */
void SendAudioVoltageToVofaPlus(void)
{
    uint32_t total_samples;
    uint32_t addr;
    uint8_t read_buf[UART_TX_BUF_SIZE];
    uint32_t remaining;
    uint32_t read_size;
    uint32_t samples_in_chunk;
    uint32_t i;
    uint16_t sample_raw;
    float sample_float;
    uint8_t *f_ptr;
    uint8_t percent;
    
    total_samples = current_flash_addr / 2;  // 总采样点数 (uint16_t)
    addr = 0;
    
    OLED_Clear();
    OLED_ShowString(0, 0, "UART Sending...", OLED_8X16);
    OLED_ShowString(0, 16, "0%", OLED_8X16);
    OLED_Update();
    
    while (addr < current_flash_addr)
    {
        remaining = current_flash_addr - addr;
        read_size = (remaining > UART_TX_BUF_SIZE) ? UART_TX_BUF_SIZE : remaining;
        
        // 从W25Q64读取一段录音数据
        W25Q64_ReadData(addr, read_buf, read_size);
        
        // 逐采样点转换并发送
        samples_in_chunk = read_size / 2;
        for (i = 0; i < samples_in_chunk; i++)
        {
            // 读取两个字节组成 uint16_t (小端: W25Q64 低地址存低字节)
            sample_raw = (uint16_t)read_buf[i * 2] | ((uint16_t)read_buf[i * 2 + 1] << 8);
            
            // 将 ADC 原始值换算为电压 (Vref=3.3V, 12bit ADC)
            sample_float = (float)sample_raw * 3.3f / 4095.0f;
            
            // 发送 float32 (4字节, 小端)
            f_ptr = (uint8_t *)&sample_float;
            DL_UART_transmitDataBlocking(UART_0_INST, f_ptr[0]);
            DL_UART_transmitDataBlocking(UART_0_INST, f_ptr[1]);
            DL_UART_transmitDataBlocking(UART_0_INST, f_ptr[2]);
            DL_UART_transmitDataBlocking(UART_0_INST, f_ptr[3]);
            
            // 发送 VOFA+ RawData 帧尾
            DL_UART_transmitDataBlocking(UART_0_INST, VOFA_FRAME_FOOTER0);
            DL_UART_transmitDataBlocking(UART_0_INST, VOFA_FRAME_FOOTER1);
            DL_UART_transmitDataBlocking(UART_0_INST, VOFA_FRAME_FOOTER2);
            DL_UART_transmitDataBlocking(UART_0_INST, VOFA_FRAME_FOOTER3);
        }
        
        addr += read_size;
        
        // 更新进度显示
        percent = (uint8_t)((addr * 100UL) / current_flash_addr);
        OLED_ShowString(0, 16, "   ", OLED_8X16);  // 清除旧数字
        OLED_ShowNum(0, 16, percent, 3, OLED_8X16);
        OLED_ShowString(24, 16, "%", OLED_8X16);
        OLED_Update();
    }
    
    // 发送完成
    OLED_Clear();
    OLED_ShowString(0, 0, "UART Done!", OLED_8X16);
    OLED_ShowNum(0, 16, total_samples, 6, OLED_8X16);
    OLED_ShowString(48, 16, "smp", OLED_8X16);
    OLED_Update();
}