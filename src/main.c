/**
 * \file		main.c
 * \brief		Includes the main function, which will be called after the start up sequence.
 *
 * \mainpage	LIDAR
 * \author		Kevin Gerber
 * \date		2014-05-05
 * \version		0.1
 *
 * \note		This is a developer preview.
 * \warning		Tests the SPI interface!
 *
 * \section Introduction
 * \section Architecture
 * \image html architecture.png
 * \section Hardware
 */

#include <stdint.h>
#include <string.h>

#include "stm32f4xx.h"
#include "bsp.h"
#include "bsp_led.h"
#include "bsp_gp22.h"

#include "bsp_serial.h"

extern int siprintf(char *buf, const char *fmt, ...);

/**
 * \brief	A blocked time delay.
 */
void delay(void) {
	volatile uint32_t ctr;

	for (ctr=0; ctr<0x3FFFF; ctr++) {

	}
}

/** Demo string to test the serial interface. */
char msg[] = "Hallo Welt! Der Text wie immer bei diesen Programmierern :) \r\n";

/** Button T0 */
static const bsp_gpioconf_t BSP_CARME_T0 = {
		RCC_AHB1Periph_GPIOC, GPIOC, GPIO_Pin_7, GPIO_Mode_IN, GPIO_PuPd_NOPULL
};

double g_CalResonatorFactor = 0.0;

/**
 * \brief	TDC Interrupt callback.
 */
void tdcIntCallback(void) {
	uint8_t success;
	uint32_t stat;
	uint32_t tdc_result;
	int32_t propagation_delay_ps;

	char buffer[128];
	uint32_t length;

	/* Read STAT and check that no timeout occurs */
	success = bsp_GP22RegRead(GP22_RD_STAT, &stat, 2);

	/* Read time delay from RES_0 */
	success = bsp_GP22RegRead(GP22_RD_RES_0, &tdc_result, 4);

	if (success) {
		propagation_delay_ps = (int32_t) ((tdc_result / 65535.0) * (250000.0)); //500000.0
		length = siprintf(buffer, "%d ps\r\n", propagation_delay_ps);
		bsp_SerialStringPut(buffer, length);
	}
}

/**
 * \brief	Main function. Will be called after the startup sequence.
 * 			The main function initialize the real time operating system and starts it.
 * \return	This function should never finished.
 */
int main(void) {
	//uint32_t dummy;

	/* Ensure all priority bits are assigned as preemption priority bits. */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	bsp_GpioInit(&BSP_CARME_T0);

	bsp_SerialInit();
	bsp_SerialStringPut(msg, strlen(msg));
	bsp_LedInit();
	bsp_GP22Init();
	bsp_GP22IntCallback(tdcIntCallback);

	/* Enable IRQ */
	__enable_irq();

//	delay();
//	bsp_GP22ReadState(&dummy);

	/* Calibrate the high speed oscillator */
	bsp_GP22SendOpcode(GP22_OP_Start_Cal_Resonator);
	delay();

	/* Start TDC measurement */
	bsp_GP22SendOpcode(GP22_OP_Init);

	/* Infinite loop */
	while (1) {

		/* Calibrate high speed clock */
		//bsp_GP22Opcode(OP_Start_Cal_Resonator);
		/* Wait for INT */
		/* Read RD_RES_0 */
		/* Correction factor = 61.035 / RES_O */

		/* Calibrate TDC */
		//bsp_GP22Opcode(OP_Start_Cal_TDC);

		/* Start TDC measurement */
		//bsp_GP22Opcode(OP_Init);

		while (!GPIO_ReadInputDataBit(BSP_CARME_T0.base, BSP_CARME_T0.pin)) {
			// Wait until T0 is pressed
		}

		/* Start TDC measurement */
		bsp_GP22SendOpcode(GP22_OP_Init);

		/* Simulate the hardware */
		bsp_LedSetOn(BSP_LED_OUT_1);
		bsp_LedSetOn(BSP_LED_OUT_2);
		bsp_LedSetOn(BSP_LED_OUT_3);
		bsp_LedSetOff(BSP_LED_OUT_1);
		bsp_LedSetOff(BSP_LED_OUT_2);
		bsp_LedSetOff(BSP_LED_OUT_3);

		delay();

		while (GPIO_ReadInputDataBit(BSP_CARME_T0.base, BSP_CARME_T0.pin)) {
			// Wait until T0 is released
		}

		delay();
	}

	return 0;
}
