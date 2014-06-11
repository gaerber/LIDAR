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
#include <string.h>

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
#include "task_comminterp.h"
#include "task_scanner.h"

/* BSP */
#include "bsp_led.h"
#include "bsp_quadenc.h"
#include "bsp_laser.h"

/* Utility */
#include "incs_azimuth.h"

/* Imported function prototypes */
extern void Reset_Handler(void);
extern int sprintf(char* str, const char *fmt, ...);


/*
 * ----------------------------------------------------------------------------
 * Private data types
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Contains all system states and configurations.
 */
typedef struct {
	/* User settings */
	uint8_t comm_echo;			/*!< Enable or disable the command echo. */
	uint8_t comm_respmsg;		/*!< Enable or disable the response message. */
	int16_t scan_bndry_left;	/*!< Configured scan area boundary left. [tenth degree] */
	int16_t scan_bndry_right;	/*!< Configured scan area boundary right. [tenth degree] */
	int16_t scan_step;			/*!< Configures step size between two measurement points. [tenth degree] */
	uint8_t scan_rate;			/*!< Configured update rate of the hole room map. [turns per second] */
	uint16_t engine_sleep;		/*!< Configured time delay before the engine is suspended in CMD mode. [ms] */

	/* System settings */
	enum {
		MODE_CMD,				/*!< Mode CMD. */
		MODE_DATA				/*!< Mode DATA. */
	} state;					/*!< System mode. */
	readcommand_t readcommand;	/*!< The read command value for the command interpreter task. */
	uint32_t azimuth_left;		/*!< Calculated scan area boundary left. [increments] */
	uint32_t azimuth_right;		/*!< Calculated scan area boundary right. [increments] */
	uint32_t azimuth_res;		/*!< Calculated step size between two measurement points. [increments] */
	uint32_t laser_pulses;		/*!< Number of laser pulses each measurement point. */
	int32_t engine_speed;		/*!< Engine speed in increments per time interval. */
} system_t;


/*
 * ----------------------------------------------------------------------------
 * Private functions prototypes
 * ----------------------------------------------------------------------------
 */
void MalfLedCallback(TimerHandle_t xTimer);
void engineStandByCallback(TimerHandle_t xTimer);
void systemCheckCallback(TimerHandle_t xTimer);

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

/**
 * \brief	Software timer handler for the engine sleep feature.
 */
TimerHandle_t timerEngineSleep;

/**
 * \brief	Software timer handler for the cycle system test.
 */
TimerHandle_t timerSystemCheck;

/**
 * \brief	The actual system state and configurations.
 */

system_t g_systemState;

/*
 * ----------------------------------------------------------------------------
 * Timer callback functions
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Timer callback function to switch off the malfunction LED after
 * 			tree seconds.
 * \param[in]	xTimer The identifier that is assigned to the timer being called.
 * 				Not used.
 */
void MalfLedCallback(TimerHandle_t xTimer) {
	/* Reset the error LED */
	bsp_LedSetOff(BSP_LED_RED);
}

/**
 * \brief	Timer callback function to stop the engine after a defines time
 * 			delay.
 * \param[in]	xTimer The identifier that is assigned to the timer being called.
 * 				Not used.
 */
void engineStandByCallback(TimerHandle_t xTimer) {
	/* Stop the engine */
	speed_t engine_speed = 0;
	xQueueSend(queueSpeed, &engine_speed, portMAX_DELAY);
}

/**
 * \brief	Timer callback function to make a system check of the data acquisition.
 * 			The Laser driver error flag will be checked.
 * \param[in]	xTimer The identifier that is assigned to the timer being called.
 * 				Not used.
 */
void systemCheckCallback(TimerHandle_t xTimer) {
	command_t command;
	uint8_t overcurrent;
	static uint8_t overcurrent_last = 1;

	/* Get the overcurrent flag */
	overcurrent = bsp_LaserOvercurrent();

	/* Check if a overcurrent is ocured */
	if (!overcurrent && overcurrent_last) {
		command.command = Malf_LaserDriver;
		if (xQueueSend(queueCommand, &command, 0) == pdTRUE) {
			overcurrent_last = overcurrent;
		}
		/* If queue is full, wait for an other check cycle */
	}
	else {
		overcurrent_last = overcurrent;
	}
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
	bsp_LedInit();

	/* Initialize the data acquisition */
	DataAcquisitionInit();

	/* Generate the task */
	xTaskCreate(taskController, TASK_CONTROLLER_NAME, TASK_CONTROLLER_STACKSIZE,
			NULL, TASK_CONTROLLER_PRIORITY, &taskControllerHandle);

	/* Generate the queue */
	queueCommand = xQueueCreate(Q_COMMAND_LENGTH, sizeof(command_t));

	/* Generate the timer */
	timerMalfunctionLed = xTimerCreate("Malf LED", 3000/portTICK_PERIOD_MS,
			pdFALSE, (void*)BSP_LED_RED, MalfLedCallback);
	timerEngineSleep = xTimerCreate("Engine sleep", 3000/portTICK_PERIOD_MS,
			pdFALSE, NULL, engineStandByCallback);
	timerSystemCheck = xTimerCreate("Sys Check", 100/portTICK_PERIOD_MS,
			pdTRUE, NULL, systemCheckCallback);
}

/**
 * \brief	Controller Task. Implementation of the controller task with his own loop.
 * \param[in]	pvParameters task parameters. Not used.
 */
void taskController(void* pvParameters) {
	command_t command;
	char str_buffer[64];
	speed_t engine_speed;

	/* Sends the welcome text */
	command.command = Sys_Welcome;
	xQueueSend(queueCommand, &command, portMAX_DELAY);

	/* initialize the system */
	command.command = Sys_Init;
	xQueueSend(queueCommand, &command, portMAX_DELAY);

	//DEMO
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
				/* Set the default system states and configurations */
				g_systemState.comm_echo = 1;
				g_systemState.comm_respmsg = 1;
				g_systemState.scan_bndry_left = DA_AZIMUTH_MIN;
				g_systemState.scan_bndry_right = DA_AZIMUTH_MAX;
				g_systemState.scan_step = DA_AZIMUTH_RES;
				g_systemState.scan_rate = DA_DEF_SCANRATE;
				g_systemState.engine_sleep = 0;
				g_systemState.state = MODE_CMD;
				g_systemState.readcommand = 1;
				g_systemState.azimuth_left = tenthdegree2increments(DA_AZIMUTH_MIN);
				g_systemState.azimuth_right = tenthdegree2increments(DA_AZIMUTH_MAX);
				g_systemState.azimuth_res = tenthdegree2increments_Relative(DA_AZIMUTH_RES);
				g_systemState.laser_pulses = DA_LASERPULSE / DA_DEF_SCANRATE;
				g_systemState.engine_speed = BSP_QUADENC_INC_PER_TURN / (DA_DEF_SCANRATE /* * REGLER_ZEITKONSTANTE_TA */);

				g_systemState.engine_speed = 20;

				/* Reads the first user command */
				//todo xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
				break;

			/* Make a system check */
			case Sys_Check:

				break;

			/* Sends the welcome text over the interface */
			case Sys_Welcome:
				sendMessage(MSG_TYPE_STATE, "LIDAR v"LIDAR_VERSION);
				sendMessage(MSG_TYPE_STATE, "BFH Thesis 2014");
				sendMessage(MSG_TYPE_STATE, "By Kevin Gerber, Marcel Baertschi");
				break;

			/* Change into the command mode */
			case UC_Cmd:
				/* Stop the data acquisition */
				DataAcquisitionStop();

				/* Stop the engine after a given time delay */
				if (g_systemState.engine_sleep > 0) {
					xTimerChangePeriod(timerEngineSleep, g_systemState.engine_sleep, portMAX_DELAY);
					xTimerStart(timerEngineSleep, portMAX_DELAY);
				}
				else {
					/* Stop the engine now */
					engine_speed = 0;
					xQueueSend(queueSpeed, &engine_speed, portMAX_DELAY);
				}

				/* Change the state */
				g_systemState.state = MODE_CMD;
				g_systemState.readcommand = g_systemState.comm_echo;

				/* Send the response message */
				sendMessage(MSG_TYPE_STATE, "cmd");

				/* Read the next user command */
				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
				break;

			/* Change into the data mode and starts the data acquisition */
			case UC_Data:
				/* Change the system state */
				g_systemState.state = MODE_DATA;
				g_systemState.readcommand = 0;

				/* Starts the engine */
				xTimerStop(timerEngineSleep, portMAX_DELAY);
				xQueueSend(queueSpeed, &g_systemState.engine_speed, portMAX_DELAY);

				/* Send the response message */
				sendMessage(MSG_TYPE_STATE, "data");

				/* todo Waits until the engine reached his speed. */
				/* Starts the data acquisition */
				DataAcquisitionStart(g_systemState.azimuth_left, g_systemState.azimuth_right,
						g_systemState.azimuth_res, g_systemState.laser_pulses);

				/* Read the next user command */
				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
				break;

			/* Reboot the system */
			case UC_Reboot:
				/* Send the response message */
				sendMessage(MSG_TYPE_STATE, "rebooting");

				/* Delay to send the output buffer */
				vTaskDelay(10);

				/* Disable all interrupts */
				__disable_irq();

				/* call the reset handler */
				Reset_Handler();
				break;

			/* Enable/disable the command echo */
			case UC_SetCommEcho:
				/* Change the system state */
				g_systemState.comm_echo = command.param.echo;
				g_systemState.readcommand = command.param.echo;

				/* Send the acknowledge to the user */
				sendMessage(MSG_TYPE_RSP, "00 aok");

				/* Read the next user command */
				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
				break;

			/* Enable/disable the response message */
			case UC_SetCommRespmsg:
				/* Change the system state */
				g_systemState.comm_respmsg = command.param.respmsg;

				/* Send the acknowledge to the user */
				sendMessage(MSG_TYPE_RSP, "00 aok");

				/* Read the next user command */
				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
				break;

			/* Configure the scan area boundary */
			case UC_SetScanBndry:
				/* Change the system state */
				g_systemState.scan_bndry_left = command.param.azimuth_bndry.left;
				g_systemState.scan_bndry_right = command.param.azimuth_bndry.right;
				g_systemState.azimuth_left = tenthdegree2increments(command.param.azimuth_bndry.left);
				g_systemState.azimuth_right = tenthdegree2increments(command.param.azimuth_bndry.right);

				/* Send the acknowledge to the user */
				sendMessage(MSG_TYPE_RSP, "00 aok");

				/* Read the next user command */
				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
				break;

			/* Configure the step size between two measurement points */
			case UC_SetScanStep:
				/* Change the system state */
				g_systemState.scan_step = command.param.azimuth_step;
				g_systemState.azimuth_res = tenthdegree2increments_Relative(command.param.azimuth_step);

				/* Send the acknowledge to the user */
				sendMessage(MSG_TYPE_RSP, "00 aok");

				/* Read the next user command */
				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
				break;

			/* Configure the update rate of the hole room map */
			case UC_SetScanRate:
				/* Change the system state */
				g_systemState.scan_rate = command.param.scan_rate;
				g_systemState.engine_speed = BSP_QUADENC_INC_PER_TURN / (command.param.scan_rate /* * REGLER_ZEITKONSTANTE_TA */);

				/* Send the acknowledge to the user */
				sendMessage(MSG_TYPE_RSP, "00 aok");

				/* Read the next user command */
				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
				break;

			/* Sets the time delay before the engine is suspended */
			case UC_SetEngineSleep:
				/* Change the system state */
				g_systemState.engine_sleep = command.param.engine_sleep;

				/* Send the acknowledge to the user */
				sendMessage(MSG_TYPE_RSP, "00 aok");

				/* Read the next user command */
				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
				break;

			/* Get all configured parameters */
			case UC_GetAll:
				/* Execute all get cases */

			/* Get the version number */
			case UC_GetVer:
				/* Print the version number */
				sendMessage(MSG_TYPE_CONF, "ver "LIDAR_VERSION);

				/* Execute all get cases */
				if (command.command != UC_GetAll) {
					/* Read the next user command */
					xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
					break;
				}

			/* Get the communication configurations */
			case UC_GetComm:
				/* Print communication echo */
				sprintf(str_buffer, "comm echo %s", g_systemState.comm_echo ? "on" : "off");
				sendMessage(MSG_TYPE_CONF, str_buffer);

				/* Print communication response message */
				sprintf(str_buffer, "comm respmsg %s", g_systemState.comm_respmsg ? "on" : "off");
				sendMessage(MSG_TYPE_CONF, str_buffer);

				/* Execute all get cases */
				if (command.command != UC_GetAll) {
					/* Read the next user command */
					xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
					break;
				}

			/* Get the scan configurations */
			case UC_GetScan:
				/* Print scan boundary */
				sprintf(str_buffer, "scan bndry %d %d", g_systemState.scan_bndry_left, g_systemState.scan_bndry_right);
				sendMessage(MSG_TYPE_CONF, str_buffer);

				/* Print scan step */
				sprintf(str_buffer, "scan step %d", g_systemState.scan_step);
				sendMessage(MSG_TYPE_CONF, str_buffer);

				/* Print scan rate */
				sprintf(str_buffer, "scan rate %d", g_systemState.scan_rate);
				sendMessage(MSG_TYPE_CONF, str_buffer);

				/* Execute all get cases */
				if (command.command != UC_GetAll) {
					/* Read the next user command */
					xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
					break;
				}

			/* Get the engine configurations */
			case UC_GetEngine:
				/* Print engine sleep */
				sprintf(str_buffer, "engien sleep %d", g_systemState.engine_sleep);
				sendMessage(MSG_TYPE_CONF, str_buffer);

				/* Read the next user command */
				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
				break;

			/* todo last: Some magic feature */
			case UC_EE:
				triggerMalfunctionLed();
				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
				break;

			/* todo last: Exit the system */
			case UC_Exit:

				break;

			/* A unknown command is received */
			case ErrUC_UnknownCommand:
				sprintf(str_buffer, "%d unknown command", 11 + command.param.error_level);
				sendMessage(MSG_TYPE_RSP, str_buffer);
				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
				break;

			/* To few arguments in the command */
			case ErrUC_TooFewArgs:
				sendMessage(MSG_TYPE_RSP, "21 too few arguments");
				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
				break;

			/* One or more arguments were in the fault data type */
			case ErrUC_FaultArgType:
				sendMessage(MSG_TYPE_RSP, "22 fault argument type");
				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
				break;

			/* One or more arguments were out of the allowed bounds */
			case ErrUC_ArgOutOfBounds:
				sendMessage(MSG_TYPE_RSP, "31 argument out of bound");
				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
				break;

			/* Command line overflow detected */
			case ErrUC_LineOverflow:
				/* Trigger the error LED */
				triggerMalfunctionLed();

				/* Send e dummy message to be sure the frame end was sent */
				sendMessage(MSG_TYPE_STATE, "");

				/* Send the error message. */
				sendMessage(MSG_TYPE_RSP, "91 command line overflow");
				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
				break;

			/* Engine overcurrent or thermal shutdown */
			case Malf_EngineDriver:
				/* Trigger the error LED */
				triggerMalfunctionLed();
			break;

			/* Laser overcurrent was detected */
			case Malf_LaserDriver:
				/* Trigger the error LED */
				triggerMalfunctionLed();
				break;

			/* Quadrature encoder malfunction was detected */
			case Malf_QuadEnc:
				/* Trigger the error LED */
				triggerMalfunctionLed();
				break;

			/* TDC stat register has an unexpected value */
			case Malf_Tdc:
				/* Trigger the error LED */
				triggerMalfunctionLed();
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
	message_t message;

	/* Build the message structure */
	message.type = msg_type;
	if (msg_type != MSG_TYPE_RSP || g_systemState.comm_respmsg) {
		strcpy(message.msg, msg);
	}
	else {
		/* Only the error number */
		message.msg[0] = msg[0];
		message.msg[1] = msg[1];
	}

	/* Send the message to the gatekeeper */
	xQueueSend(queueMessage, &message, portMAX_DELAY);
}

/**
 * \brief	Set the malfunction LED for 3 seconds.
 * 			This Function is retriggerable.
 */
void triggerMalfunctionLed(void) {
	/* Starts the timer */
	xTimerStart(timerMalfunctionLed, portMAX_DELAY);

	/* Set the LED */
	bsp_LedSetOn(BSP_LED_RED);
}


/**
 * @}
 */

/**
 * @}
 */
