/**
 * \file		task_scanner.c
 * \brief		Controls the rotation of the mirror.
 * \date		2014-05-28
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	app
 * @{
 *
 * \addtogroup	task_scanner
 * @{
 */

#include <stdint.h>

/* RTOS */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Application */
#include "task_scanner.h"

/*
 * ----------------------------------------------------------------------------
 * Private functions prototypes
 * ----------------------------------------------------------------------------
 */
void taskScanner(void* pvParameters);


/*
 * ----------------------------------------------------------------------------
 * Task synchronization
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Command interpreter task handle.
 */
TaskHandle_t taskScannerHandle;

/**
 * \brief	Queue to set a new setpiont value of the rotation speed.
 */
QueueHandle_t queueSpeed;


/*
 * ----------------------------------------------------------------------------
 * Implementation
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Scanner task initialization. Generates the task and initialize the hardware.
 */
void taskScannerInit(void) {

	/* Initialize the engine */

	/* Initialize the quadrature encoder */

	/* Generate the task */
	xTaskCreate(taskScanner, TASK_SCANNER_NAME, TASK_SCANNER_STACKSIZE,
			NULL, TASK_SCANNER_PRIORITY, &taskScannerHandle);

	/* Generate the queue */
	queueSpeed = xQueueCreate(Q_SPEED_LENGTH, sizeof(speed_t));
}


/**
 * \brief	Scanner Task. Implementation of the scanner task with his own loop.
 */
void taskScanner(void* pvParameters) {

	/* Loop forever */
	for (;;) {

	}

	/* Never reach this point */
}


/**
 * @}
 */

/**
 * @}
 */
