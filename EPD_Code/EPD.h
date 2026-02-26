#ifndef _EPD_H_
#define _EPD_H_

#include "ti_msp_dl_config.h"

#define EPD_W	152 
#define EPD_H	152

#define WHITE 0xFF
#define BLACK 0x00

//#define EPD_SCL_Clr() GPIO_ResetBits(EPD_SCL_GPIO_PORT,EPD_SCL_GPIO_PIN)
//#define EPD_SCL_Set() GPIO_SetBits(EPD_SCL_GPIO_PORT,EPD_SCL_GPIO_PIN)

//#define EPD_SDA_Clr() GPIO_ResetBits(EPD_SDA_GPIO_PORT,EPD_SDA_GPIO_PIN)
//#define EPD_SDA_Set() GPIO_SetBits(EPD_SDA_GPIO_PORT,EPD_SDA_GPIO_PIN)

#define EPD_RES_Clr() DL_GPIO_clearPins(EPD_RES_PORT,EPD_RES_PIN)
#define EPD_RES_Set() DL_GPIO_setPins(EPD_RES_PORT,EPD_RES_PIN)

#define EPD_DC_Clr() DL_GPIO_clearPins(EPD_DC_PORT,EPD_DC_PIN)
#define EPD_DC_Set() DL_GPIO_setPins(EPD_DC_PORT,EPD_DC_PIN)

#define EPD_CS_Clr() DL_GPIO_clearPins(EPD_CS_PORT,EPD_CS_PIN)
#define EPD_CS_Set() DL_GPIO_setPins(EPD_CS_PORT,EPD_CS_PIN)
//#define EPD_ReadBusy()  ((DL_GPIO_readPins(EPD_BUSY_PORT, EPD_BUSY_PIN) & EPD_BUSY_PIN) ? 1 : 0)

void EPD_GPIOInit(void);
void EPD_READBUSY(void);
void EPD_HW_RESET(void);
void EPD_Update(void);
void EPD_DeepSleep(void);
void EPD_Init(void);
void EPD_Display_Clear(void);
void EPD_Display(const uint8_t *imageBW,const uint8_t *imageR);
#endif
