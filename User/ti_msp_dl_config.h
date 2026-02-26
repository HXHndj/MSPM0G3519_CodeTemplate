/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G351X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G351X
#define CONFIG_MSPM0G3519

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)


#define GPIO_HFXT_PORT                                                     GPIOA
#define GPIO_HFXIN_PIN                                             DL_GPIO_PIN_5
#define GPIO_HFXIN_IOMUX                                         (IOMUX_PINCM10)
#define GPIO_HFXOUT_PIN                                            DL_GPIO_PIN_6
#define GPIO_HFXOUT_IOMUX                                        (IOMUX_PINCM11)
#define CPUCLK_FREQ                                                     80000000



/* Defines for TIMER_0 */
#define TIMER_0_INST                                                     (TIMA0)
#define TIMER_0_INST_IRQHandler                                 TIMA0_IRQHandler
#define TIMER_0_INST_INT_IRQN                                   (TIMA0_INT_IRQn)
#define TIMER_0_INST_LOAD_VALUE                                          (6249U)




/* Defines for I2C_0 */
#define I2C_0_INST                                                          I2C0
#define I2C_0_INST_IRQHandler                                    I2C0_IRQHandler
#define I2C_0_INST_INT_IRQN                                        I2C0_INT_IRQn
#define I2C_0_BUS_SPEED_HZ                                                100000
#define GPIO_I2C_0_SDA_PORT                                                GPIOA
#define GPIO_I2C_0_SDA_PIN                                         DL_GPIO_PIN_8
#define GPIO_I2C_0_IOMUX_SDA                                     (IOMUX_PINCM19)
#define GPIO_I2C_0_IOMUX_SDA_FUNC                      IOMUX_PINCM19_PF_I2C0_SDA
#define GPIO_I2C_0_SCL_PORT                                                GPIOA
#define GPIO_I2C_0_SCL_PIN                                         DL_GPIO_PIN_9
#define GPIO_I2C_0_IOMUX_SCL                                     (IOMUX_PINCM20)
#define GPIO_I2C_0_IOMUX_SCL_FUNC                      IOMUX_PINCM20_PF_I2C0_SCL


/* Defines for UART_0 */
#define UART_0_INST                                                        UART0
#define UART_0_INST_FREQUENCY                                           40000000
#define UART_0_INST_IRQHandler                                  UART0_IRQHandler
#define UART_0_INST_INT_IRQN                                      UART0_INT_IRQn
#define GPIO_UART_0_RX_PORT                                                GPIOA
#define GPIO_UART_0_TX_PORT                                                GPIOA
#define GPIO_UART_0_RX_PIN                                         DL_GPIO_PIN_1
#define GPIO_UART_0_TX_PIN                                         DL_GPIO_PIN_0
#define GPIO_UART_0_IOMUX_RX                                      (IOMUX_PINCM2)
#define GPIO_UART_0_IOMUX_TX                                      (IOMUX_PINCM1)
#define GPIO_UART_0_IOMUX_RX_FUNC                       IOMUX_PINCM2_PF_UART0_RX
#define GPIO_UART_0_IOMUX_TX_FUNC                       IOMUX_PINCM1_PF_UART0_TX
#define UART_0_BAUD_RATE                                                  (9600)
#define UART_0_IBRD_40_MHZ_9600_BAUD                                       (260)
#define UART_0_FBRD_40_MHZ_9600_BAUD                                        (27)




/* Defines for SPI_0 */
#define SPI_0_INST                                                         SPI0
#define SPI_0_INST_IRQHandler                                   SPI0_IRQHandler
#define SPI_0_INST_INT_IRQN                                       SPI0_INT_IRQn
#define GPIO_SPI_0_PICO_PORT                                              GPIOB
#define GPIO_SPI_0_PICO_PIN                                       DL_GPIO_PIN_2
#define GPIO_SPI_0_IOMUX_PICO                                   (IOMUX_PINCM15)
#define GPIO_SPI_0_IOMUX_PICO_FUNC                   IOMUX_PINCM15_PF_SPI0_PICO
/* GPIO configuration for SPI_0 */
#define GPIO_SPI_0_SCLK_PORT                                              GPIOB
#define GPIO_SPI_0_SCLK_PIN                                       DL_GPIO_PIN_3
#define GPIO_SPI_0_IOMUX_SCLK                                   (IOMUX_PINCM16)
#define GPIO_SPI_0_IOMUX_SCLK_FUNC                   IOMUX_PINCM16_PF_SPI0_SCLK



/* Port definition for Pin Group LED */
#define LED_PORT                                                         (GPIOC)

/* Defines for PIN_1: GPIOC.6 with pinCMx 84 on package pin 63 */
#define LED_PIN_1_PIN                                            (DL_GPIO_PIN_6)
#define LED_PIN_1_IOMUX                                          (IOMUX_PINCM84)
/* Defines for RES: GPIOB.23 with pinCMx 51 on package pin 70 */
#define OLED_RES_PORT                                                    (GPIOB)
#define OLED_RES_PIN                                            (DL_GPIO_PIN_23)
#define OLED_RES_IOMUX                                           (IOMUX_PINCM51)
/* Defines for CS: GPIOC.9 with pinCMx 87 on package pin 66 */
#define OLED_CS_PORT                                                     (GPIOC)
#define OLED_CS_PIN                                              (DL_GPIO_PIN_9)
#define OLED_CS_IOMUX                                            (IOMUX_PINCM87)
/* Defines for DC: GPIOC.8 with pinCMx 86 on package pin 65 */
#define OLED_DC_PORT                                                     (GPIOC)
#define OLED_DC_PIN                                              (DL_GPIO_PIN_8)
#define OLED_DC_IOMUX                                            (IOMUX_PINCM86)
/* Port definition for Pin Group KeyBoard */
#define KeyBoard_PORT                                                    (GPIOB)

/* Defines for H1: GPIOB.6 with pinCMx 23 on package pin 30 */
#define KeyBoard_H1_PIN                                          (DL_GPIO_PIN_6)
#define KeyBoard_H1_IOMUX                                        (IOMUX_PINCM23)
/* Defines for H2: GPIOB.7 with pinCMx 24 on package pin 31 */
#define KeyBoard_H2_PIN                                          (DL_GPIO_PIN_7)
#define KeyBoard_H2_IOMUX                                        (IOMUX_PINCM24)
/* Defines for H3: GPIOB.8 with pinCMx 25 on package pin 32 */
#define KeyBoard_H3_PIN                                          (DL_GPIO_PIN_8)
#define KeyBoard_H3_IOMUX                                        (IOMUX_PINCM25)
/* Defines for H4: GPIOB.9 with pinCMx 26 on package pin 33 */
#define KeyBoard_H4_PIN                                          (DL_GPIO_PIN_9)
#define KeyBoard_H4_IOMUX                                        (IOMUX_PINCM26)
/* Defines for V1: GPIOB.20 with pinCMx 48 on package pin 67 */
#define KeyBoard_V1_PIN                                         (DL_GPIO_PIN_20)
#define KeyBoard_V1_IOMUX                                        (IOMUX_PINCM48)
/* Defines for V2: GPIOB.24 with pinCMx 52 on package pin 71 */
#define KeyBoard_V2_PIN                                         (DL_GPIO_PIN_24)
#define KeyBoard_V2_IOMUX                                        (IOMUX_PINCM52)
/* Defines for V3: GPIOB.25 with pinCMx 56 on package pin 75 */
#define KeyBoard_V3_PIN                                         (DL_GPIO_PIN_25)
#define KeyBoard_V3_IOMUX                                        (IOMUX_PINCM56)
/* Defines for V4: GPIOB.27 with pinCMx 58 on package pin 77 */
#define KeyBoard_V4_PIN                                         (DL_GPIO_PIN_27)
#define KeyBoard_V4_IOMUX                                        (IOMUX_PINCM58)


/* Defines for TRNG */
/*
 * The TRNG interrupt is part of INT_GROUP1. Refer to the TRM for more details
 * on interrupt grouping
 */
#define TRNG_INT_IRQN                                            (TRNG_INT_IRQn)
#define TRNG_INT_IIDX                            (DL_INTERRUPT_GROUP1_IIDX_TRNG)



/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_TIMER_0_init(void);
void SYSCFG_DL_I2C_0_init(void);
void SYSCFG_DL_UART_0_init(void);
void SYSCFG_DL_SPI_0_init(void);

void SYSCFG_DL_TRNG_init(void);

bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
