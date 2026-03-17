#ifndef __EEPROM_WR_H
#define __EEPROM_WR_H

#include "ti_msp_dl_config.h"
#include "eeprom_emulation_type_a.h"
#include "MenuSys.h" // 包含 ShopList 结构体定义

#define SECTOR_SIZE (1024)
#define NUMBER_OF_WRITES ((SECTOR_SIZE / EEPROM_EMULATION_DATA_SIZE) + 1)

// 声明 EEPROM 状态和缓冲区，方便外部查询
extern uint32_t EEPROMEmulationState; 
extern uint32_t EEPROMEmulationBuffer[EEPROM_EMULATION_DATA_SIZE / sizeof(uint32_t)];

void Flash_EEPROM_Init(void);
void Flash_SaveProduct(ShopList *product, uint32_t *state);
void Flash_LoadProduct(ShopList *product);

#endif