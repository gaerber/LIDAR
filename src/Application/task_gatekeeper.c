/**
 * \file		task_gatekeeper.c
 * \brief		Sends all messages over the serial interface to the user.
 * \date		2014-05-28
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	app
 * @{
 *
 * \addtogroup	task_gatekeeper
 * @{
 */

#include <stdint.h>

/* RTOS */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Application */
#include "task_gatekeeper.h"


/*
 * ----------------------------------------------------------------------------
 * Private functions prototypes
 * ----------------------------------------------------------------------------
 */
void taskGatekeeper(void* pvParameters);


/*
 * ----------------------------------------------------------------------------
 * Task synchronization
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Command interpreter task handle.
 */
TaskHandle_t taskGatekeeperHandle;

/**
 * \brief	Message queue. All message goes over this queue to the user.
 */
QueueHandle_t queueMessage;


/*
 * ----------------------------------------------------------------------------
 * Implementation
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Gatekeeper task initialization. Generates the task and initialize
 * 			the hardware.
 */
void taskGatekeeperInit(void) {

	/* Initialize the serial interface */

	/* Generate the task */
	xTaskCreate(taskGatekeeper, TASK_GATEKEEPER_NAME, TASK_GATEKEEPER_STACKSIZE,
			NULL, TASK_GATEKEEPER_PRIORITY, &taskGatekeeperHandle);

	/* Generate the queue */
	queueMessage = xQueueCreate(Q_MESSAGE_LENGTH, sizeof(message_t));
}


/**
 * \brief	Gatekeeper Task. Implementation of the gatekeeper task with his own loop.
 */
void taskGatekeeper(void* pvParameters) {

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
