/*
 * Copyright (c) 2023, Texas Instruments Incorporated
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
 *  ============ ti_msp_dl_config.c =============
 *  Configured MSPM0 DriverLib module definitions
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G351X
 *  by the SysConfig tool.
 */

#include "ti_msp_dl_config.h"

DL_TimerA_backupConfig gTIMER_ADCBackup;
DL_TimerA_backupConfig gTIMER_KEYBackup;
DL_SPI_backupConfig gSPI_0Backup;
DL_SPI_backupConfig gSPI_W25Q64Backup;
DL_TRNG_backupConfig gTRNGBackup;

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform any initialization needed before using any board APIs
 */
SYSCONFIG_WEAK void SYSCFG_DL_init(void)
{
    SYSCFG_DL_initPower();
    SYSCFG_DL_GPIO_init();
    /* Module-Specific Initializations*/
    SYSCFG_DL_SYSCTL_init();
    SYSCFG_DL_TIMER_ADC_init();
    SYSCFG_DL_TIMER_KEY_init();
    SYSCFG_DL_TIMER_DAC_init();
    SYSCFG_DL_UART_0_init();
    SYSCFG_DL_SPI_0_init();
    SYSCFG_DL_SPI_W25Q64_init();
    SYSCFG_DL_ADC12_0_init();
    SYSCFG_DL_DMA_init();
    SYSCFG_DL_TRNG_init();
    SYSCFG_DL_DAC12_init();
    /* Ensure backup structures have no valid state */
	gTIMER_ADCBackup.backupRdy 	= false;
	gTIMER_KEYBackup.backupRdy 	= false;

	gSPI_0Backup.backupRdy 	= false;
	gSPI_W25Q64Backup.backupRdy 	= false;
	gTRNGBackup.backupRdy 	= false;

}
/*
 * User should take care to save and restore register configuration in application.
 * See Retention Configuration section for more details.
 */
SYSCONFIG_WEAK bool SYSCFG_DL_saveConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_TimerA_saveConfiguration(TIMER_ADC_INST, &gTIMER_ADCBackup);
	retStatus &= DL_TimerA_saveConfiguration(TIMER_KEY_INST, &gTIMER_KEYBackup);
	retStatus &= DL_SPI_saveConfiguration(SPI_0_INST, &gSPI_0Backup);
	retStatus &= DL_SPI_saveConfiguration(SPI_W25Q64_INST, &gSPI_W25Q64Backup);
	retStatus &= DL_TRNG_saveConfiguration(TRNG, &gTRNGBackup);

    return retStatus;
}


SYSCONFIG_WEAK bool SYSCFG_DL_restoreConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_TimerA_restoreConfiguration(TIMER_ADC_INST, &gTIMER_ADCBackup, false);
	retStatus &= DL_TimerA_restoreConfiguration(TIMER_KEY_INST, &gTIMER_KEYBackup, false);
	retStatus &= DL_SPI_restoreConfiguration(SPI_0_INST, &gSPI_0Backup);
	retStatus &= DL_SPI_restoreConfiguration(SPI_W25Q64_INST, &gSPI_W25Q64Backup);
	retStatus &= DL_TRNG_restoreConfiguration(TRNG, &gTRNGBackup);

    return retStatus;
}

SYSCONFIG_WEAK void SYSCFG_DL_initPower(void)
{
    DL_GPIO_reset(GPIOA);
    DL_GPIO_reset(GPIOB);
    DL_GPIO_reset(GPIOC);
    DL_TimerA_reset(TIMER_ADC_INST);
    DL_TimerA_reset(TIMER_KEY_INST);
    DL_TimerG_reset(TIMER_DAC_INST);
    DL_UART_Main_reset(UART_0_INST);
    DL_SPI_reset(SPI_0_INST);
    DL_SPI_reset(SPI_W25Q64_INST);
    DL_ADC12_reset(ADC12_0_INST);

    DL_TRNG_reset(TRNG);
    DL_DAC12_reset(DAC0);

    DL_GPIO_enablePower(GPIOA);
    DL_GPIO_enablePower(GPIOB);
    DL_GPIO_enablePower(GPIOC);
    DL_TimerA_enablePower(TIMER_ADC_INST);
    DL_TimerA_enablePower(TIMER_KEY_INST);
    DL_TimerG_enablePower(TIMER_DAC_INST);
    DL_UART_Main_enablePower(UART_0_INST);
    DL_SPI_enablePower(SPI_0_INST);
    DL_SPI_enablePower(SPI_W25Q64_INST);
    DL_ADC12_enablePower(ADC12_0_INST);

    DL_TRNG_enablePower(TRNG);
    DL_DAC12_enablePower(DAC0);
    delay_cycles(POWER_STARTUP_DELAY);
}

SYSCONFIG_WEAK void SYSCFG_DL_GPIO_init(void)
{

    DL_GPIO_initPeripheralAnalogFunction(GPIO_HFXIN_IOMUX);
    DL_GPIO_initPeripheralAnalogFunction(GPIO_HFXOUT_IOMUX);

    DL_GPIO_initPeripheralOutputFunction(
        GPIO_UART_0_IOMUX_TX, GPIO_UART_0_IOMUX_TX_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_UART_0_IOMUX_RX, GPIO_UART_0_IOMUX_RX_FUNC);

    DL_GPIO_initPeripheralOutputFunction(
        GPIO_SPI_0_IOMUX_SCLK, GPIO_SPI_0_IOMUX_SCLK_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_SPI_0_IOMUX_PICO, GPIO_SPI_0_IOMUX_PICO_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_SPI_W25Q64_IOMUX_SCLK, GPIO_SPI_W25Q64_IOMUX_SCLK_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_SPI_W25Q64_IOMUX_PICO, GPIO_SPI_W25Q64_IOMUX_PICO_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_SPI_W25Q64_IOMUX_POCI, GPIO_SPI_W25Q64_IOMUX_POCI_FUNC);

    DL_GPIO_initDigitalOutput(LED_PIN_1_IOMUX);

    DL_GPIO_initDigitalOutput(W25Q64_W25Q64_CS_IOMUX);

    DL_GPIO_initDigitalOutput(OLED_RES_IOMUX);

    DL_GPIO_initDigitalOutput(OLED_CS_IOMUX);

    DL_GPIO_initDigitalOutput(OLED_DC_IOMUX);

    DL_GPIO_initDigitalOutput(KeyBoard_H1_IOMUX);

    DL_GPIO_initDigitalOutput(KeyBoard_H2_IOMUX);

    DL_GPIO_initDigitalOutput(KeyBoard_H3_IOMUX);

    DL_GPIO_initDigitalOutput(KeyBoard_H4_IOMUX);

    DL_GPIO_initDigitalInputFeatures(KeyBoard_V1_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(KeyBoard_V2_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(KeyBoard_V3_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(KeyBoard_V4_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_clearPins(GPIOB, OLED_RES_PIN |
		KeyBoard_H1_PIN |
		KeyBoard_H2_PIN |
		KeyBoard_H3_PIN |
		KeyBoard_H4_PIN);
    DL_GPIO_setPins(GPIOB, W25Q64_W25Q64_CS_PIN);
    DL_GPIO_enableOutput(GPIOB, W25Q64_W25Q64_CS_PIN |
		OLED_RES_PIN |
		KeyBoard_H1_PIN |
		KeyBoard_H2_PIN |
		KeyBoard_H3_PIN |
		KeyBoard_H4_PIN);
    DL_GPIO_clearPins(GPIOC, OLED_CS_PIN |
		OLED_DC_PIN);
    DL_GPIO_setPins(GPIOC, LED_PIN_1_PIN);
    DL_GPIO_enableOutput(GPIOC, LED_PIN_1_PIN |
		OLED_CS_PIN |
		OLED_DC_PIN);

}


static const DL_SYSCTL_SYSPLLConfig gSYSPLLConfig = {
    .inputFreq              = DL_SYSCTL_SYSPLL_INPUT_FREQ_32_48_MHZ,
	.rDivClk2x              = 1,
	.rDivClk1               = 0,
	.rDivClk0               = 0,
	.enableCLK2x            = DL_SYSCTL_SYSPLL_CLK2X_DISABLE,
	.enableCLK1             = DL_SYSCTL_SYSPLL_CLK1_DISABLE,
	.enableCLK0             = DL_SYSCTL_SYSPLL_CLK0_ENABLE,
	.sysPLLMCLK             = DL_SYSCTL_SYSPLL_MCLK_CLK0,
	.sysPLLRef              = DL_SYSCTL_SYSPLL_REF_HFCLK,
	.qDiv                   = 3,
	.pDiv                   = DL_SYSCTL_SYSPLL_PDIV_1
};
SYSCONFIG_WEAK void SYSCFG_DL_SYSCTL_init(void)
{

	//Low Power Mode is configured to be SLEEP0
    DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);
    DL_SYSCTL_setFlashWaitState(DL_SYSCTL_FLASH_WAIT_STATE_2);

    
	DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);
	/* Set default configuration */
	DL_SYSCTL_disableHFXT();
	DL_SYSCTL_disableSYSPLL();
    DL_SYSCTL_setHFCLKSourceHFXTParams(DL_SYSCTL_HFXT_RANGE_32_48_MHZ,10, false);
    DL_SYSCTL_configSYSPLL((DL_SYSCTL_SYSPLLConfig *) &gSYSPLLConfig);
    DL_SYSCTL_setULPCLKDivider(DL_SYSCTL_ULPCLK_DIV_2);
    DL_SYSCTL_setHFCLKDividerForMFPCLK(DL_SYSCTL_HFCLK_MFPCLK_DIVIDER_10);
    DL_SYSCTL_enableMFCLK();
    DL_SYSCTL_enableMFPCLK();
	DL_SYSCTL_setMFPCLKSource(DL_SYSCTL_MFPCLK_SOURCE_HFCLK);
    DL_SYSCTL_setMCLKSource(SYSOSC, HSCLK, DL_SYSCTL_HSCLK_SOURCE_SYSPLL);

}



/*
 * Timer clock configuration to be sourced by BUSCLK /  (80000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   80000000 Hz = 80000000 Hz / (1 * (0 + 1))
 */
static const DL_TimerA_ClockConfig gTIMER_ADCClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale    = 0U,
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * TIMER_ADC_INST_LOAD_VALUE = (62.5 us * 80000000 Hz) - 1
 */
static const DL_TimerA_TimerConfig gTIMER_ADCTimerConfig = {
    .period     = TIMER_ADC_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC,
    .startTimer = DL_TIMER_STOP,
};

SYSCONFIG_WEAK void SYSCFG_DL_TIMER_ADC_init(void) {

    DL_TimerA_setClockConfig(TIMER_ADC_INST,
        (DL_TimerA_ClockConfig *) &gTIMER_ADCClockConfig);

    DL_TimerA_initTimerMode(TIMER_ADC_INST,
        (DL_TimerA_TimerConfig *) &gTIMER_ADCTimerConfig);
    DL_TimerA_enableShadowFeatures(TIMER_ADC_INST);
    DL_TimerA_enableClock(TIMER_ADC_INST);


    DL_TimerA_enableEvent(TIMER_ADC_INST, DL_TIMERA_EVENT_ROUTE_1, (DL_TIMERA_EVENT_ZERO_EVENT));

    DL_TimerA_setPublisherChanID(TIMER_ADC_INST, DL_TIMERA_PUBLISHER_INDEX_0, TIMER_ADC_INST_PUB_0_CH);



}

/*
 * Timer clock configuration to be sourced by BUSCLK /  (10000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   1000000 Hz = 10000000 Hz / (8 * (9 + 1))
 */
static const DL_TimerA_ClockConfig gTIMER_KEYClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_8,
    .prescale    = 9U,
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * TIMER_KEY_INST_LOAD_VALUE = (10 ms * 1000000 Hz) - 1
 */
static const DL_TimerA_TimerConfig gTIMER_KEYTimerConfig = {
    .period     = TIMER_KEY_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_TIMER_KEY_init(void) {

    DL_TimerA_setClockConfig(TIMER_KEY_INST,
        (DL_TimerA_ClockConfig *) &gTIMER_KEYClockConfig);

    DL_TimerA_initTimerMode(TIMER_KEY_INST,
        (DL_TimerA_TimerConfig *) &gTIMER_KEYTimerConfig);
    DL_TimerA_enableInterrupt(TIMER_KEY_INST , DL_TIMERA_INTERRUPT_ZERO_EVENT);
    DL_TimerA_enableClock(TIMER_KEY_INST);





}

/*
 * Timer clock configuration to be sourced by BUSCLK /  (40000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   40000000 Hz = 40000000 Hz / (1 * (0 + 1))
 */
static const DL_TimerG_ClockConfig gTIMER_DACClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale    = 0U,
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * TIMER_DAC_INST_LOAD_VALUE = (62.5 us * 40000000 Hz) - 1
 */
static const DL_TimerG_TimerConfig gTIMER_DACTimerConfig = {
    .period     = TIMER_DAC_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC,
    .startTimer = DL_TIMER_STOP,
};

SYSCONFIG_WEAK void SYSCFG_DL_TIMER_DAC_init(void) {

    DL_TimerG_setClockConfig(TIMER_DAC_INST,
        (DL_TimerG_ClockConfig *) &gTIMER_DACClockConfig);

    DL_TimerG_initTimerMode(TIMER_DAC_INST,
        (DL_TimerG_TimerConfig *) &gTIMER_DACTimerConfig);
    DL_TimerG_enableClock(TIMER_DAC_INST);


    DL_TimerG_enableEvent(TIMER_DAC_INST, DL_TIMERG_EVENT_ROUTE_1, (DL_TIMERG_EVENT_ZERO_EVENT));

    DL_TimerG_setPublisherChanID(TIMER_DAC_INST, DL_TIMERG_PUBLISHER_INDEX_0, TIMER_DAC_INST_PUB_0_CH);



}


static const DL_UART_Main_ClockConfig gUART_0ClockConfig = {
    .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config gUART_0Config = {
    .mode        = DL_UART_MAIN_MODE_NORMAL,
    .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity      = DL_UART_MAIN_PARITY_NONE,
    .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
};

SYSCONFIG_WEAK void SYSCFG_DL_UART_0_init(void)
{
    DL_UART_Main_setClockConfig(UART_0_INST, (DL_UART_Main_ClockConfig *) &gUART_0ClockConfig);

    DL_UART_Main_init(UART_0_INST, (DL_UART_Main_Config *) &gUART_0Config);
    /*
     * Configure baud rate by setting oversampling and baud rate divisors.
     *  Target baud rate: 115200
     *  Actual baud rate: 115190.78
     */
    DL_UART_Main_setOversampling(UART_0_INST, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(UART_0_INST, UART_0_IBRD_40_MHZ_115200_BAUD, UART_0_FBRD_40_MHZ_115200_BAUD);


    /* Configure Interrupts */
    DL_UART_Main_enableInterrupt(UART_0_INST,
                                 DL_UART_MAIN_INTERRUPT_RX);


    DL_UART_Main_enable(UART_0_INST);
}

static const DL_SPI_Config gSPI_0_config = {
    .mode        = DL_SPI_MODE_CONTROLLER,
    .frameFormat = DL_SPI_FRAME_FORMAT_MOTO3_POL0_PHA0,
    .parity      = DL_SPI_PARITY_NONE,
    .dataSize    = DL_SPI_DATA_SIZE_8,
    .bitOrder    = DL_SPI_BIT_ORDER_MSB_FIRST,
};

static const DL_SPI_ClockConfig gSPI_0_clockConfig = {
    .clockSel    = DL_SPI_CLOCK_BUSCLK,
    .divideRatio = DL_SPI_CLOCK_DIVIDE_RATIO_1
};

SYSCONFIG_WEAK void SYSCFG_DL_SPI_0_init(void) {
    DL_SPI_setClockConfig(SPI_0_INST, (DL_SPI_ClockConfig *) &gSPI_0_clockConfig);

    DL_SPI_init(SPI_0_INST, (DL_SPI_Config *) &gSPI_0_config);

    /* Configure Controller mode */
    /*
     * Set the bit rate clock divider to generate the serial output clock
     *     outputBitRate = (spiInputClock) / ((1 + SCR) * 2)
     *     10000000 = (80000000)/((1 + 3) * 2)
     */
    DL_SPI_setBitRateSerialClockDivider(SPI_0_INST, 3);
    /* Set RX and TX FIFO threshold levels */
    DL_SPI_setFIFOThreshold(SPI_0_INST, DL_SPI_RX_FIFO_LEVEL_1_2_FULL, DL_SPI_TX_FIFO_LEVEL_1_2_EMPTY);

    /* Enable module */
    DL_SPI_enable(SPI_0_INST);
}
static const DL_SPI_Config gSPI_W25Q64_config = {
    .mode        = DL_SPI_MODE_CONTROLLER,
    .frameFormat = DL_SPI_FRAME_FORMAT_MOTO3_POL0_PHA0,
    .parity      = DL_SPI_PARITY_NONE,
    .dataSize    = DL_SPI_DATA_SIZE_8,
    .bitOrder    = DL_SPI_BIT_ORDER_MSB_FIRST,
};

static const DL_SPI_ClockConfig gSPI_W25Q64_clockConfig = {
    .clockSel    = DL_SPI_CLOCK_BUSCLK,
    .divideRatio = DL_SPI_CLOCK_DIVIDE_RATIO_1
};

SYSCONFIG_WEAK void SYSCFG_DL_SPI_W25Q64_init(void) {
    DL_SPI_setClockConfig(SPI_W25Q64_INST, (DL_SPI_ClockConfig *) &gSPI_W25Q64_clockConfig);

    DL_SPI_init(SPI_W25Q64_INST, (DL_SPI_Config *) &gSPI_W25Q64_config);

    /* Configure Controller mode */
    /*
     * Set the bit rate clock divider to generate the serial output clock
     *     outputBitRate = (spiInputClock) / ((1 + SCR) * 2)
     *     8000000 = (80000000)/((1 + 4) * 2)
     */
    DL_SPI_setBitRateSerialClockDivider(SPI_W25Q64_INST, 4);
    /* Set RX and TX FIFO threshold levels */
    DL_SPI_setFIFOThreshold(SPI_W25Q64_INST, DL_SPI_RX_FIFO_LEVEL_1_2_FULL, DL_SPI_TX_FIFO_LEVEL_1_2_EMPTY);

    /* Enable module */
    DL_SPI_enable(SPI_W25Q64_INST);
}

/* ADC12_0 Initialization */
static const DL_ADC12_ClockConfig gADC12_0ClockConfig = {
    .clockSel       = DL_ADC12_CLOCK_SYSOSC,
    .divideRatio    = DL_ADC12_CLOCK_DIVIDE_2,
    .freqRange      = DL_ADC12_CLOCK_FREQ_RANGE_24_TO_32,
};
SYSCONFIG_WEAK void SYSCFG_DL_ADC12_0_init(void)
{
    DL_ADC12_setClockConfig(ADC12_0_INST, (DL_ADC12_ClockConfig *) &gADC12_0ClockConfig);
    DL_ADC12_initSingleSample(ADC12_0_INST,
        DL_ADC12_REPEAT_MODE_ENABLED, DL_ADC12_SAMPLING_SOURCE_AUTO, DL_ADC12_TRIG_SRC_EVENT,
        DL_ADC12_SAMP_CONV_RES_12_BIT, DL_ADC12_SAMP_CONV_DATA_FORMAT_UNSIGNED);
    DL_ADC12_configConversionMem(ADC12_0_INST, ADC12_0_ADCMEM_0,
        DL_ADC12_INPUT_CHAN_0, DL_ADC12_REFERENCE_VOLTAGE_VDDA_VSSA, DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0, DL_ADC12_AVERAGING_MODE_ENABLED,
        DL_ADC12_BURN_OUT_SOURCE_DISABLED, DL_ADC12_TRIGGER_MODE_TRIGGER_NEXT, DL_ADC12_WINDOWS_COMP_MODE_DISABLED);
    DL_ADC12_setPowerDownMode(ADC12_0_INST,DL_ADC12_POWER_DOWN_MODE_MANUAL);
    DL_ADC12_configHwAverage(ADC12_0_INST,DL_ADC12_HW_AVG_NUM_ACC_8,DL_ADC12_HW_AVG_DEN_DIV_BY_8);
    DL_ADC12_setSampleTime0(ADC12_0_INST,32);
    DL_ADC12_setSampleTime1(ADC12_0_INST,1600);
    DL_ADC12_enableDMA(ADC12_0_INST);
    DL_ADC12_setDMASamplesCnt(ADC12_0_INST,1);
    DL_ADC12_enableDMATrigger(ADC12_0_INST,(DL_ADC12_DMA_MEM0_RESULT_LOADED));
    DL_ADC12_setSubscriberChanID(ADC12_0_INST,ADC12_0_INST_SUB_CH);
    DL_ADC12_enableConversions(ADC12_0_INST);
}

static const DL_DMA_Config gDMA_CH0Config = {
    .transferMode   = DL_DMA_FULL_CH_REPEAT_SINGLE_TRANSFER_MODE,
    .extendedMode   = DL_DMA_NORMAL_MODE,
    .destIncrement  = DL_DMA_ADDR_INCREMENT,
    .srcIncrement   = DL_DMA_ADDR_UNCHANGED,
    .destWidth      = DL_DMA_WIDTH_HALF_WORD,
    .srcWidth       = DL_DMA_WIDTH_HALF_WORD,
    .trigger        = ADC12_0_INST_DMA_TRIGGER,
    .triggerType    = DL_DMA_TRIGGER_TYPE_EXTERNAL,
};

SYSCONFIG_WEAK void SYSCFG_DL_DMA_CH0_init(void)
{
    DL_DMA_clearInterruptStatus(DMA, DL_DMA_INTERRUPT_CHANNEL0);
    DL_DMA_enableInterrupt(DMA, DL_DMA_INTERRUPT_CHANNEL0);
    DL_DMA_clearInterruptStatus(DMA, DL_DMA_FULL_CH_INTERRUPT_EARLY_CHANNEL0);
    DL_DMA_enableInterrupt(DMA, DL_DMA_FULL_CH_INTERRUPT_EARLY_CHANNEL0);
    DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, 4096);
    DL_DMA_initChannel(DMA, DMA_CH0_CHAN_ID , (DL_DMA_Config *) &gDMA_CH0Config);
    DL_DMA_Full_Ch_setEarlyInterruptThreshold(DMA, DMA_CH0_CHAN_ID, DL_DMA_EARLY_INTERRUPT_THRESHOLD_HALF);
}
static const DL_DMA_Config gDMA_CH1Config = {
    .transferMode   = DL_DMA_FULL_CH_REPEAT_SINGLE_TRANSFER_MODE,
    .extendedMode   = DL_DMA_NORMAL_MODE,
    .destIncrement  = DL_DMA_ADDR_UNCHANGED,
    .srcIncrement   = DL_DMA_ADDR_INCREMENT,
    .destWidth      = DL_DMA_WIDTH_HALF_WORD,
    .srcWidth       = DL_DMA_WIDTH_HALF_WORD,
    .trigger        = DMA_CH1_TRIGGER_SEL_FSUB_0,
    .triggerType    = DL_DMA_TRIGGER_TYPE_EXTERNAL,
};

SYSCONFIG_WEAK void SYSCFG_DL_DMA_CH1_init(void)
{
    DL_DMA_clearInterruptStatus(DMA, DL_DMA_INTERRUPT_CHANNEL1);
    DL_DMA_enableInterrupt(DMA, DL_DMA_INTERRUPT_CHANNEL1);
    DL_DMA_clearInterruptStatus(DMA, DL_DMA_FULL_CH_INTERRUPT_EARLY_CHANNEL1);
    DL_DMA_enableInterrupt(DMA, DL_DMA_FULL_CH_INTERRUPT_EARLY_CHANNEL1);
    DL_DMA_setTransferSize(DMA, DMA_CH1_CHAN_ID, 4096);
    DL_DMA_initChannel(DMA, DMA_CH1_CHAN_ID , (DL_DMA_Config *) &gDMA_CH1Config);
    DL_DMA_Full_Ch_setEarlyInterruptThreshold(DMA, DMA_CH1_CHAN_ID, DL_DMA_EARLY_INTERRUPT_THRESHOLD_HALF);
}
SYSCONFIG_WEAK void SYSCFG_DL_DMA_init(void){
    DL_DMA_setSubscriberChanID(DMA, DL_DMA_SUBSCRIBER_INDEX_0, 2);
    SYSCFG_DL_DMA_CH0_init();
    SYSCFG_DL_DMA_CH1_init();
}


SYSCONFIG_WEAK void SYSCFG_DL_TRNG_init(void)
{
    DL_TRNG_setClockDivider(TRNG, DL_TRNG_CLOCK_DIVIDE_8);

    DL_TRNG_sendCommand(TRNG, DL_TRNG_CMD_NORM_FUNC);
    while (!DL_TRNG_isCommandDone(TRNG))
        ;
    DL_TRNG_clearInterruptStatus(TRNG, DL_TRNG_INTERRUPT_CMD_DONE_EVENT);

    DL_TRNG_setDecimationRate(TRNG, DL_TRNG_DECIMATION_RATE_4);
}

static const DL_DAC12_Config gDAC12Config = {
    .outputEnable              = DL_DAC12_OUTPUT_ENABLED,
    .resolution                = DL_DAC12_RESOLUTION_12BIT,
    .representation            = DL_DAC12_REPRESENTATION_BINARY,
    .voltageReferenceSource    = DL_DAC12_VREF_SOURCE_VDDA_VSSA,
    .amplifierSetting          = DL_DAC12_AMP_ON,
    .fifoEnable                = DL_DAC12_FIFO_DISABLED,
    .fifoTriggerSource         = DL_DAC12_FIFO_TRIGGER_SAMPLETIMER,
    .dmaTriggerEnable          = DL_DAC12_DMA_TRIGGER_DISABLED,
    .dmaTriggerThreshold       = DL_DAC12_FIFO_THRESHOLD_ONE_QTR_EMPTY,
    .sampleTimeGeneratorEnable = DL_DAC12_SAMPLETIMER_DISABLE,
    .sampleRate                = DL_DAC12_SAMPLES_PER_SECOND_500,
};
SYSCONFIG_WEAK void SYSCFG_DL_DAC12_init(void)
{
    DL_DAC12_init(DAC0, (DL_DAC12_Config *) &gDAC12Config);
    DL_DAC12_enable(DAC0);
}

