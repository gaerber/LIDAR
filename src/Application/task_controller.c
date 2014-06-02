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
#include "semphr.h"
#include "timers.h"

/* Application */
#include "task_controller.h"
#include "data_acquisition.h"
#include "task_gatekeeper.h"

/* BSP */
#include "bsp_led.h"

extern void Reset_Handler(void);


/*
 * ----------------------------------------------------------------------------
 * Private functions prototypes
 * ----------------------------------------------------------------------------
 */
void MalfLedCallback(TimerHandle_t xTimer);
void taskController(void* pvParameters);
void sendMessage(char msg_typw, const char* msg);
void triggerMalfunctionLed(void);


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
 * Private variables
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Software timer handler for the malfunction LED.
 */
TimerHandle_t timerMalfunctionLed;


/*
 * ----------------------------------------------------------------------------
 * Timer callback functions
 * ----------------------------------------------------------------------------
 */

void MalfLedCallback(TimerHandle_t xTimer) {
	/* Reset the LED */
}


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
	//bsp_LedInit();

	/* Initialize the data acquisition */
	DataAcquisitionInit();

	/* Generate the task */
	xTaskCreate(taskController, TASK_CONTROLLER_NAME, TASK_CONTROLLER_STACKSIZE,
			NULL, TASK_CONTROLLER_PRIORITY, &taskControllerHandle);

	/* Generate the queue */
	queueCommand = xQueueCreate(Q_COMMAND_LENGTH, sizeof(command_t));

	/* Generate the timer */
	xTimerCreateTimerTask();
	timerMalfunctionLed = xTimerCreate("Malf LED", 3000/portTICK_PERIOD_MS,
			pdFALSE, (void*)BSP_LED_RED, MalfLedCallback);
}

/**
 * \brief	Controller Task. Implementation of the controller task with his own loop.
 */
void taskController(void* pvParameters) {
	command_t command;
	/* Timeout synchronization */

	//DEMO
	command.command = Sys_Init;
	xQueueSend(queueCommand, &command, portMAX_DELAY);
	command.command = UC_Data;
	xQueueSend(queueCommand, &command, portMAX_DELAY);

	/* Loop forever */
	for (;;) {
		/* Wait for an event */
		if (xQueueReceive(queueCommand, &command, 100) == pdTRUE) {

			/* Resolve the command */
			switch (command.command) {

			/* Initialize the system. Called after the system start */
			case Sys_Init:

				break;

			/* Make a system check */
			case Sys_Check:

				break;

			/* Sends the welcome text over the interface */
			case Sys_Welcome:
				sendMessage(MSG_TYPE_STATE, "LIDAR v1.0");
				sendMessage(MSG_TYPE_STATE, "BFH Thesis 2014");
				sendMessage(MSG_TYPE_STATE, "By Kevin Gerber, Marcel Baertschi");
				break;

			/* Change into the command mode */
			case UC_Cmd:

				break;

			/* Change into the data mode and starts the data acquisition */
			case UC_Data:
				DataAcquisitionStart(500,1500,100,5);
				break;

			/* Reboot the system */
			case UC_Reboot:
				/* Disable all interrupts */
				/* call the reset handler */
				Reset_Handler();
				break;

			/* Enable/disable the command echo */
			case UC_SetCommEcho:

				break;

			/* Enable/disable the response message */
			case UC_SetCommRespmsg:

				break;

			/* Configure the scan area boundary */
			case US_SetScanBndry:

				break;

			/* Configure the step size between two measurement points */
			case US_SetScanStep:

				break;

			/* Configure the update rate of the hole room map */
			case US_SetScanRate:

				break;

			/* Sets the time delay before the engine is suspended */
			case US_SetEngineSleep:

				break;

			/* Get all configured parameters */
			case US_GetAll:

				break;

			/* Get the version number */
			case US_GetVer:

				break;

			/* Get the communication configurations */
			case US_GetComm:

				break;

			/* Get the scan configurations */
			case US_GetScan:

				break;

			/* Get the engine configurations */
			case US_GetEngine:

				break;

			/* Some magic feature */
			case US_EE:

				break;

			/* A unknown command is received */
			case ErrUC_UnknownCommand:

				break;

			/* To few arguments in the command */
			case ErrUC_TooFewArgs:

				break;

			/* One or more arguments were in the fault data type */
			case ErrUC_FaultArgType:

				break;

			/* One or more arguments were out of the allowed bounds */
			case ErrUC_ArgOutOfBounds:

				break;

			/* Command line overflow detected */
			case Malf_LineOverflow:

				break;

			/* Engine overcurrent or thermal shutdown */
			case Malf_EngineDriver:

			break;

			/* Laser overcurrent was detected */
			case Malf_LaserDriver:

				break;

			/* Quadrature encoder malfunction was detected */
			case Malf_QuadEnc:

				break;

			/* TDC stat register has an unexpected value */
			case Malf_Tdc:

				break;

			/* Command error */
			default:
				triggerMalfunctionLed();
				sendMessage(MSG_TYPE_STATE, "Controller Task Error");
				break;
			}
		}
	}

	/* Never reach this point */
}


/**
 * \brief	Send a message to the user over the gatekeeper task.
 * \param[in]	msg_type is the message type.
 * \param[in]	msg is the string.
 */
void sendMessage(char msg_type, const char* msg) {

}

/**
 * \brief	Set the malfunction LED for 3 seconds.
 * 			This Function is retriggerable.
 */
void triggerMalfunctionLed(void) {
	/* Starts the timer */
	xTimerStart(timerMalfunctionLed, portMAX_DELAY);

	/* Set the LED */

}


/**
 * @}
 */

/**
 * @}
 */
