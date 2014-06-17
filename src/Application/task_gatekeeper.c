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
#include "task_controller.h"

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
 * \brief	Message queue. All normal message goes over this queue to the user.
 */
QueueHandle_t queueMessage;

/**
 * \brief	Data message queue with the information about the room map.
 */
QueueHandle_t queueMessageData;

/**
 * \brief	Queue set to trigger a message.
 */
QueueSetHandle_t queueMessageSet;

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
	queueMessageData = xQueueCreate(Q_MESSAGE_DATA_LENGTH, sizeof(char[DATA_MESSAGE_STRING_LENGTH]));

	/* Create the message queue set */
	queueMessageSet = xQueueCreateSet(Q_MESSAGE_LENGTH + Q_MESSAGE_DATA_LENGTH);
	xQueueAddToSet(queueMessage, queueMessageSet);
	xQueueAddToSet(queueMessageData, queueMessageSet);

	/* Generate the mutual exclusion */
	mutexTxCircBuf = xSemaphoreCreateMutex();
	xSemaphoreGive(mutexTxCircBuf);
}


/**
 * \brief	Gatekeeper Task. Implementation of the gatekeeper task with his own loop.
 * \param[in]	pvParameters task parameters. Not used.
 */
void taskGatekeeper(void* pvParameters) {
	QueueSetMemberHandle_t xActivatedMember;

	message_t message;
	char message_data[DATA_MESSAGE_STRING_LENGTH + 1] = { '\0' };
	char *ptr;

	char selector;
	uint32_t i;
	static const char frame_end[] = MSG_FRAME_END;

	command_t command;
	uint32_t timeout;

	/* Loop forever */
	for (;;) {
		/* Get the message, which has to be sent */
		xActivatedMember = xQueueSelectFromSet(queueMessageSet, portMAX_DELAY);

		/* Check the type of the message */
		if (xActivatedMember == queueMessageData) {
			/* A data message */
			xQueueReceive(queueMessageData, message_data, 0);
			selector = MSG_TYPE_DATA;
			ptr = message_data;
		}
		else if (xActivatedMember == queueMessage) {
			/* A normal message */
			xQueueReceive(queueMessage, &message, 0);
			selector = message.type;
			ptr = message.msg;
		}
		else {
			/* Error event */
			xActivatedMember = NULL;
		}

		if (xActivatedMember) {
			/* Sets the timeout */
			timeout = 20;

			/* Takes the mutual exclusion to write into the circular buffer */
			xSemaphoreTake(mutexTxCircBuf, portMAX_DELAY);

			/* Send the message type selector */
			while (!bsp_SerialCharPut(selector) && timeout > 0) {
				/* No space available in the circular buffer */
				vTaskDelay(10/portTICK_PERIOD_MS);
				timeout--;
			}

			/* Send the string to the TX output buffer */
			while (*ptr != '\0' && timeout > 0) {
				while (!bsp_SerialCharPut(*ptr++)) {
					/* No space available in the circular buffer */
					vTaskDelay(10/portTICK_PERIOD_MS);
					timeout--;
				}
			}

			/* Send the end of the message frame */
			for (i=0; i<sizeof(frame_end)-1; i++) {
				while (!bsp_SerialCharPut(frame_end[i]) && timeout > 0) {
					/* No space available in the circular buffer */
					vTaskDelay(10/portTICK_PERIOD_MS);
					timeout--;
				}
			}

			/* Check if there was a timeout */
			if (timeout == 0) {
				/* Sent the error event */
				command.command = Marf_Serial;
				xQueueSend(queueCommand, &command, portMAX_DELAY);
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
