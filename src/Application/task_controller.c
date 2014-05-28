/**
 * \file		task_controller.c
 * \brief		Contains the controller task.
 * \date		2014-05-28
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	app
 * @{
 *
 * \addtogroup	task_controller
* @{
 */

#include <stdint.h>

/* RTOS */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Application */
#include "task_controller.h"


/*
 * ----------------------------------------------------------------------------
 * Private functions prototypes
 * ----------------------------------------------------------------------------
 */
void taskController(void* pvParameters);


/*
 * ----------------------------------------------------------------------------
 * Task synchronization
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Controller task handle.
 */
TaskHandle_t taskControllerHandle;

/**
 * \brief	Queue with the received and resolved commands from the user.
 */
QueueHandle_t queueCommand;


/*
 * ----------------------------------------------------------------------------
 * Implementation
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Controller task initialization. Generates the task and initialize the hardware.
 */
void taskControllerInit(void) {

	/* Initialize the LEDs */


	/* Generate the task */
	xTaskCreate(taskController, TASK_CONTROLLER_NAME, TASK_CONTROLLER_STACKSIZE,
			NULL, TASK_CONTROLLER_PRIORITY, &taskControllerHandle);

	/* Generate the queue */
	queueCommand = xQueueCreate(Q_COMMAND_LENGTH, sizeof(command_t));
}

/**
 * \brief	Controller Task. Implementation of the controller task with his own loop.
 */
void taskController(void* pvParameters) {

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
