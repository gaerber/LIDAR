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
 * \warning		Tests the laser pulse generator!
 *
 * \section Introduction
 * \section Architecture
 * \image html architecture.png
 * \section Hardware
 */

#include <stdint.h>
#include <string.h>

#include "stm32f4xx.h"
#include "bsp_laser.h"

extern int siprintf(char *buf, const char *fmt, ...);

/**
 * \brief	A blocked time delay.
 */
void delay(void) {
	volatile uint32_t ctr;

	for (ctr=0; ctr<0x3FFFFF; ctr++) {

	}
}

/** Demo string to test the serial interface. */
char msg[] = "Hallo Welt! Der Text wie immer bei diesen Programmierern :) \r\n";

/** Button T0 */
static const bsp_gpioconf_t BSP_BUTTON_USER = {
		RCC_AHB1Periph_GPIOA, GPIOA, GPIO_Pin_0, GPIO_Mode_IN, GPIO_PuPd_NOPULL
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
 * \brief	Azimuth callback function.
 * \param[in] azimuth is the current azimuth.
 */
void QuadencCallback(uint32_t azimuth) {
	bsp_QuadencSetCapture((azimuth + 10) % BSP_QUADENC_INC_PER_TURN);
	bsp_LedSetToggle(BSP_LED_RED);
}

/**
 * \brief	Quadrature encoder error hook, called if increments were loosing.
 */
void bsp_QuadencRoterrorHook(void) {
	bsp_LedSetOn(BSP_LED_ORANGE);
}

/**
 * \brief	Main function. Will be called after the startup sequence.
 * 			The main function initialize the real time operating system and starts it.
 * \return	This function should never finished.
 */
int main(void) {
	uint32_t ctr=1;

	/* Ensure all priority bits are assigned as preemption priority bits. */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	/* User button */
	bsp_GpioInit(&BSP_BUTTON_USER);

	/* LEDs */
	bsp_LedInit();

//	bsp_SerialInit();
//	bsp_SerialStringPut(msg, strlen(msg));
//	bsp_LedInit();
//	bsp_GP22Init();
	bsp_LaserInit();
//
	bsp_QuadencInit();
	bsp_QuadencPosCallback(QuadencCallback);
	bsp_QuadencSetCapture(100);

	/* Enable IRQ */
	__enable_irq();

	delay();

	/* Infinite loop */
	while (1) {
		bsp_LaserPulse(ctr++);
		delay();
	}
#endif

	return 0;
}
