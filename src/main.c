/**
 * \file		main.c
 * \brief		Includes the main function, which will be called after the start
 * 				up sequence.
 *
 * \mainpage	LIDAR
 * \author		Kevin Gerber
 * \date		2014-05-23
 * \version		0.1
 *
 * \note		This is a developer preview.
 * \warning		Application template.
 *
 * \section Introduction
 * \section Architecture
 * \image html architecture.png
 * \section Hardware
 *
 * \addtogroup	app
 * \brief		This is the main module of the application. It contains the main
 * 				function, which is called after the start up sequence and the
 * 				system initialization.
 * @{
 */

#include <stdint.h>

/* CMSIS */
#include "stm32f4xx.h"

/* RTOS */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include "memPoolService.h"

/* Application */
#include "task_comminterp.h"
#include "task_controller.h"
#include "task_gatekeeper.h"
#include "task_scanner.h"
#include "task_dataprocessing.h"

#include "task_dataacquisition.h"

/**
 * \brief	A blocked time delay.
 */
void delay(void) {
	volatile uint32_t ctr;

	for (ctr=0; ctr<0x3FFFF; ctr++) {

	}
}


/** Button T0 */
//static const bsp_gpioconf_t BSP_BUTTON_USER = {
//		RCC_AHB1Periph_GPIOA, GPIOA, GPIO_Pin_0, GPIO_Mode_IN, GPIO_PuPd_NOPULL
//};


/**
 * \brief	Main function. Will be called after the startup sequence.
 * 			The main function initialize the real time operating system and starts it.
 * \return	This function should never finished.
 */
int main(void) {
	/* Ensure all priority bits are assigned as preemption priority bits. */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	/* Initialize the software timer task */
	xTimerCreateTimerTask();

	/* Initialize all application tasks */
	taskCommInterpInit();
	taskControllerInit();
	taskGatekeeperInit();
	taskScannerInit();
	taskDataProcessingInit();

	taskDataAcquisitionInit();

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Never reach this point */
	return 0;
}

/**
 * @}
 */
