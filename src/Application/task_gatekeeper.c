/**
 * \file		task_gatekeeper.c
 * \brief		Sends all messages over the serial interface to the user.
 * \date		2014-05-29
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
#include "semphr.h"

/* Application */
#include "task_gatekeeper.h"

/* BSP */
#include "bsp_serial.h"


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

/**
 * \brief	Mutual exclusion to get access to the transmission circular buffer.
 */
SemaphoreHandle_t mutexTxCircBuf;


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
	bsp_SerialInit();

	/* Generate the task */
	xTaskCreate(taskGatekeeper, TASK_GATEKEEPER_NAME, TASK_GATEKEEPER_STACKSIZE,
			NULL, TASK_GATEKEEPER_PRIORITY, &taskGatekeeperHandle);

	/* Generate the queue */
	queueMessage = xQueueCreate(Q_MESSAGE_LENGTH, sizeof(message_t));

	/* Generate the mutual exclusion */
	mutexTxCircBuf = xSemaphoreCreateMutex();
	xSemaphoreGive(mutexTxCircBuf);
}


/**
 * \brief	Gatekeeper Task. Implementation of the gatekeeper task with his own loop.
 */
void taskGatekeeper(void* pvParameters) {
	message_t message;
	char *ptr;

	uint32_t i;
	static const char frame_end[] = MSG_FRAME_END;

	/* Loop forever */
	for (;;) {
		/* Get the message, which has to be sent */
		if (xQueueReceive(queueMessage, &message, portMAX_DELAY) == pdTRUE) {
			/* Takes the mutual exclusion to write into the circular buffer */
			xSemaphoreTake(mutexTxCircBuf, portMAX_DELAY);

			/* Send the message type selector */
			while (!bsp_SerialCharPut(message.type)) {
				/* No space available in the circular buffer */
				vTaskDelay(10/portTICK_PERIOD_MS);
			}

			/* Send the string to the TX output buffer */
			ptr = message.msg;
			while (*ptr != '\0') {
				while (!bsp_SerialCharPut(*ptr++)) {
					/* No space available in the circular buffer */
					vTaskDelay(10/portTICK_PERIOD_MS);
				}
			}

			/* Send the end of the message frame */
			for (i=0; i<sizeof(frame_end)-1; i++) {
				while (!bsp_SerialCharPut(frame_end[i])) {
					/* No space available in the circular buffer */
					vTaskDelay(10/portTICK_PERIOD_MS);
				}
			}

			/* Release the mutual exclusion */
			xSemaphoreGive(mutexTxCircBuf);
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
