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
#include "data_acquisition.h"

/* BSP */
#include "bsp_led.h"


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
	bsp_LedInit();

	/* Initialize the data acquisition */
	DataAcquisitionInit();

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
	command_t command;
	/* Timeout synchronization */

	//DEMO
	command.command = Init;
	xQueueSend(queueCommand, &command, portMAX_DELAY);
	command.command = Start;
	xQueueSend(queueCommand, &command, portMAX_DELAY);

	/* Loop forever */
	for (;;) {
		/* Wait for an event */
		if (xQueueReceive(queueCommand, &command, 100) == pdTRUE) {
			switch (command.command) {
			/* Initialization */
			case Init:

				break;

			/* Starts the data acquisition */
			case Start:
				DataAcquisitionStart(500,1500,100,5);
				break;
			}
		}
	}

	/* Never reach this point */
}


/**
 * @}
 */

/**
 * @}
 */
