/**
 * \file		task_comminterp.c
 * \brief		Interpret the received commands from the user.
 * \date		2014-05-28
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	app
 * @{
 *
 * \addtogroup	task_comminterp
 * @{
 */

#include <stdint.h>

/* RTOS */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Application */
#include "task_comminterp.h"


/*
 * ----------------------------------------------------------------------------
 * Private functions prototypes
 * ----------------------------------------------------------------------------
 */
void taskCommInterp(void* pvParameters);


/*
 * ----------------------------------------------------------------------------
 * Task synchronization
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Command interpreter task handle.
 */
TaskHandle_t taskCommInterpHandle;

/**
 * \brief	Queue with the order to read a new command. The parameter describe
 * 			if the communication echo is enabled or disabled.
 */
QueueHandle_t queueReadCommand;


/*
 * ----------------------------------------------------------------------------
 * Implementation
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Controller task initialization. Generates the task and initialize the hardware.
 */
void taskCommInterpInit(void) {

	/* No hardware initialization needed */

	/* Generate the task */
	xTaskCreate(taskCommInterp, TASK_COMMINTERP_NAME, TASK_COMMINTERP_STACKSIZE,
			NULL, TASK_COMMINTERP_PRIORITY, &taskCommInterpHandle);

	/* Generate the queue */
	queueReadCommand = xQueueCreate(Q_READCOMMAND_LENGTH, sizeof(readcommand_t));
}


/**
 * \brief	Command interpreter Task. Implementation of the command interpreter task with his own loop.
 */
void taskCommInterp(void* pvParameters) {

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
