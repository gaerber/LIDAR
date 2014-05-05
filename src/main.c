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
 * \warning		Tests the serial interface!
 *
 * \section Introduction
 * \section Architecture
 * \image html architecture.png
 * \section Hardware
 */

#include <stdint.h>
#include <string.h>

#include "bsp_serial.h"


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
	volatile uint32_t i;
	uint32_t length;
	uint32_t written;

	/* Ensure all priority bits are assigned as preemption priority bits. */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);


	bsp_SerialInit();

	/* Enable IRQ */
	__enable_irq();

//	for (i=0; i<0x3FFFFF; i++) {
//
//	}

	/* Infinite loop */
	while (1) {
		length = strlen(msg);
		written = 0;

//		for (written=0; written<length; ) {
//			if (bsp_SerialCharPut(msg[written])) {
//				written++;
//			}
//			else {
//				i++;
//			}
//		}

		do {
			written += bsp_SerialStringPut(&(msg[written]), length - written);
			if (written < length) {
				i++;
			}
		}
		while (written != length);

		delay();
	}

	return 0;
}
