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
 * \warning		Tests the LED interface!
 *
 * \section Introduction
 * \section Architecture
 * \image html architecture.png
 * \section Hardware
 */

#include <stdint.h>
#include <string.h>

#include "stm32f4xx.h"
#include "bsp_spi.h"


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

/**
 * \brief	Main function. Will be called after the startup sequence.
 * 			The main function initialize the real time operating system and starts it.
 * \return	This function should never finished.
 */
int main(void) {
	uint8_t data[] = {0x55, 0xAA, 0x00, 0xFF};
	uint32_t ctr = 0;

	/* Ensure all priority bits are assigned as preemption priority bits. */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	bsp_SPIInit();

	/* Enable IRQ */
	__enable_irq();

	/* Infinite loop */
	while (1) {
		if (!bsp_SPITransmit(BSP_SPI_CS_GP22, data, 4, NULL)) {
			ctr++;
		}
	}

	return 0;
}
