#include "EEPROM_WR.h"
#include <string.h> // 需要用到 memset 和 memcpy

uint32_t EEPROMEmulationState = 0;
uint32_t EEPROMEmulationBuffer[EEPROM_EMULATION_DATA_SIZE / sizeof(uint32_t)] = {0};

// EEPROM 初始化封装
void Flash_EEPROM_Init(void)
{
    EEPROMEmulationState = EEPROM_TypeA_init(EEPROMEmulationBuffer);
    if (EEPROMEmulationState != EEPROM_EMULATION_INIT_OK) {
        __BKPT(0); // 初始化失败，停在这里排查
    }
}

/**
 * @brief  将商品信息保存到 Flash
 * @param  product: 指向你要保存的 ShopList 结构体
 * @param  state:   传出参数，返回写入状态
 */
void Flash_SaveProduct(ShopList *product, uint32_t *state)
{
    // 1. 检查是否需要擦除扇区
    if (gEEPROMTypeAEraseFlag == 1) 
    {
        if (EEPROM_TypeA_eraseLastSector() == false)
        {
            *state = EEPROM_EMULATION_WRITE_ERROR;
            return;
        }
        gEEPROMTypeAEraseFlag = 0;
    }

    // 2. 核心步骤：先清空 Buffer，防止写入内存中的随机垃圾数据
    memset(EEPROMEmulationBuffer, 0, sizeof(EEPROMEmulationBuffer));

    // 3. 将结构体数据拷贝到 RAM 缓冲区
    memcpy((uint8_t *)EEPROMEmulationBuffer, (uint8_t *)product, sizeof(ShopList));

    // 4. 将缓冲区数据写入 Flash
    *state = EEPROM_TypeA_writeData(EEPROMEmulationBuffer);

    // 5. 写入失败触发断点
    if (*state != EEPROM_EMULATION_WRITE_OK) 
    {
        __BKPT(0); 
    }
}

/**
 * @brief  从 Flash 读取商品信息
 * @param  product: 指向你要接收数据的 ShopList 结构体
 */
void Flash_LoadProduct(ShopList *product)
{
    // 1. 从 Flash 读取最新的记录到 RAM 缓冲区
    EEPROM_TypeA_readData(EEPROMEmulationBuffer);

    // 2. 将读取到的数据拷贝回结构体
    memcpy((uint8_t *)product, (uint8_t *)EEPROMEmulationBuffer, sizeof(ShopList));
}