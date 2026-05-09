#ifndef __W25Q64_H
#define __W25Q64_H

#include "W25Q64_Ins.h"
#include "ti_msp_dl_config.h"

void W25Q64_Init(void);
void W25Q64_ReadID(uint8_t *MID, uint16_t *DID);
void W25Q64_PageProgram(uint32_t Address, uint8_t *DataArray, uint16_t Count);
void W25Q64_SectorErase(uint32_t Address);
void W25Q64_ReadData(uint32_t Address, uint8_t *DataArray, uint32_t Count);
void W25Q64_BufferWrite(uint32_t Address, uint8_t *DataArray, uint32_t Count);
void W25Q64_BlockErase_64KB(uint32_t Address);
void W25Q64_ChipErase(void);

#endif
