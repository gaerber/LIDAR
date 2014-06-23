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
#include "task_dataacquisition.h"
#include "task_gatekeeper.h"
#include "task_comminterp.h"
#include "task_scanner.h"
#include "task_ee.h"

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
void stopDataAcquisition(void);


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
 * \brief	Queue with the received and resolved commands from the user and all
 * 			other system message/malfunction.
 */
QueueHandle_t queueEvent;


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
	bsp_LedSetOff(LED_MALFUNCTION);
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

	/* Generate the task */
	xTaskCreate(taskController, TASK_CONTROLLER_NAME, TASK_CONTROLLER_STACKSIZE,
			NULL, TASK_CONTROLLER_PRIORITY, &taskControllerHandle);

	/* Generate the queue */
	queueEvent = xQueueCreate(Q_COMMAND_LENGTH, sizeof(event_t));

	/* Generate the timer */
	timerMalfunctionLed = xTimerCreate("Malf LED", 3000/portTICK_PERIOD_MS,
			pdFALSE, (void*)LED_MALFUNCTION, MalfLedCallback);
}

/**
 * \brief	Controller Task. Implementation of the controller task with his own loop.
 * \param[in]	pvParameters task parameters. Not used.
 */
void taskController(void* pvParameters) {
	event_t event;
	char str_buffer[64];
	dataacquisition_t data_acquisition_config;
	uint16_t tdc_hits;
	uint8_t hits_error;

	/* Sends the welcome text */
	event.event = Sys_Welcome;
	xQueueSend(queueEvent, &event, portMAX_DELAY);

	/* initialize the system */
	event.event = Sys_Init;
	xQueueSend(queueEvent, &event, portMAX_DELAY);

	/* Loop forever */
	for (;;) {
		/* Wait for an event */
		if (xQueueReceive(queueEvent, &event, 100) == pdTRUE) {

			/* Resolve the event */
			switch (event.event) {

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

				/* Reads the first user command */
				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
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
				if (g_systemState.state == MODE_DATA) {
					/* Stop the data acquisition */
					data_acquisition_config.state = DATA_ACQUISITION_DISABLE;
					data_acquisition_config.param.engine_sleep = g_systemState.engine_sleep;
					xQueueSend(queueDataAcquisition, &data_acquisition_config, portMAX_DELAY);

					/* Change the state */
					g_systemState.state = MODE_CMD;
					g_systemState.readcommand = g_systemState.comm_echo;
				}

				/* Reset the LED */
				bsp_LedSetOff(LED_LASER_OPERATION);

				/* Send the response message */
				sendMessage(MSG_TYPE_STATE, "cmd");

				/* Read the next user command */
				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
				break;

			/* Change into the data mode and starts the data acquisition */
			case UC_Data:
				if (g_systemState.state == MODE_CMD) {
					/* Change the system state */
					g_systemState.state = MODE_DATA;
					g_systemState.readcommand = 0;

					/* Starts the data acquisition */
					data_acquisition_config.state = DATA_ACQUISITION_ENABLE;
					data_acquisition_config.param.scan.bndry_left = g_systemState.scan_bndry_left;
					data_acquisition_config.param.scan.bndry_right = g_systemState.scan_bndry_right;
					data_acquisition_config.param.scan.step = g_systemState.scan_step;
					data_acquisition_config.param.scan.rate = g_systemState.scan_rate;
					xQueueSend(queueDataAcquisition, &data_acquisition_config, portMAX_DELAY);

					/* Set the LED */
					bsp_LedSetOn(LED_LASER_OPERATION);

					/* Send the response message */
					sendMessage(MSG_TYPE_STATE, "data");
				}

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
				if (g_systemState.state == MODE_CMD) {
					/* Change the system state */
					g_systemState.comm_echo = event.param.echo;
					g_systemState.readcommand = event.param.echo;

					/* Send the acknowledge to the user */
					sendMessage(MSG_TYPE_RSP, "00 aok");
				}

				/* Read the next user command */
				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
				break;

			/* Enable/disable the response message */
			case UC_SetCommRespmsg:
				if (g_systemState.state == MODE_CMD) {
					/* Change the system state */
					g_systemState.comm_respmsg = event.param.respmsg;

					/* Send the acknowledge to the user */
					sendMessage(MSG_TYPE_RSP, "00 aok");
				}

				/* Read the next user command */
				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
				break;

			/* Configure the scan area boundary */
			case UC_SetScanBndry:
				if (g_systemState.state == MODE_CMD) {
					/* Change the system state */
					g_systemState.scan_bndry_left = event.param.azimuth_bndry.left;
					g_systemState.scan_bndry_right = event.param.azimuth_bndry.right;

					/* Send the acknowledge to the user */
					sendMessage(MSG_TYPE_RSP, "00 aok");
				}

				/* Read the next user command */
				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
				break;

			/* Configure the step size between two measurement points */
			case UC_SetScanStep:
				if (g_systemState.state == MODE_CMD) {
					/* Change the system state */
					g_systemState.scan_step = event.param.azimuth_step;

					/* Send the acknowledge to the user */
					sendMessage(MSG_TYPE_RSP, "00 aok");
				}

				/* Read the next user command */
				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
				break;

			/* Configure the update rate of the hole room map */
			case UC_SetScanRate:
				if (g_systemState.state == MODE_CMD) {
					/* Change the system state */
					g_systemState.scan_rate = event.param.scan_rate;

					/* Send the acknowledge to the user */
					sendMessage(MSG_TYPE_RSP, "00 aok");
				}

				/* Read the next user command */
				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
				break;

			/* Sets the time delay before the engine is suspended */
			case UC_SetEngineSleep:
				if (g_systemState.state == MODE_CMD) {
					/* Change the system state */
					g_systemState.engine_sleep = event.param.engine_sleep;

					/* Send the acknowledge to the user */
					sendMessage(MSG_TYPE_RSP, "00 aok");
				}

				/* Read the next user command */
				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
				break;

			/* Get all configured parameters */
			case UC_GetAll:
				/* Execute all get cases */

			/* Get the version number */
			case UC_GetVer:
				if (g_systemState.state == MODE_CMD) {
					/* Print the version number */
					sendMessage(MSG_TYPE_CONF, "ver "LIDAR_VERSION);
				}

				/* Execute all get cases */
				if (event.event != UC_GetAll) {
					/* Read the next user command */
					xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
					break;
				}

			/* Get the communication configurations */
			case UC_GetComm:
				if (g_systemState.state == MODE_CMD) {
					/* Print communication echo */
					sprintf(str_buffer, "comm echo %s", g_systemState.comm_echo ? "on" : "off");
					sendMessage(MSG_TYPE_CONF, str_buffer);

					/* Print communication response message */
					sprintf(str_buffer, "comm respmsg %s", g_systemState.comm_respmsg ? "on" : "off");
					sendMessage(MSG_TYPE_CONF, str_buffer);
				}

				/* Execute all get cases */
				if (event.event != UC_GetAll) {
					/* Read the next user command */
					xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
					break;
				}

			/* Get the scan configurations */
			case UC_GetScan:
				if (g_systemState.state == MODE_CMD) {
					/* Print scan boundary */
					sprintf(str_buffer, "scan bndry %d %d", g_systemState.scan_bndry_left, g_systemState.scan_bndry_right);
					sendMessage(MSG_TYPE_CONF, str_buffer);

					/* Print scan step */
					sprintf(str_buffer, "scan step %d", g_systemState.scan_step);
					sendMessage(MSG_TYPE_CONF, str_buffer);

					/* Print scan rate */
					sprintf(str_buffer, "scan rate %d", g_systemState.scan_rate);
					sendMessage(MSG_TYPE_CONF, str_buffer);
				}

				/* Execute all get cases */
				if (event.event != UC_GetAll) {
					/* Read the next user command */
					xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
					break;
				}

			/* Get the engine configurations */
			case UC_GetEngine:
				if (g_systemState.state == MODE_CMD) {
					/* Print engine sleep */
					sprintf(str_buffer, "engien sleep %d", g_systemState.engine_sleep);
					sendMessage(MSG_TYPE_CONF, str_buffer);
				}

				/* Read the next user command */
				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
				break;

			/* todo last: Some magic feature */
			case UC_EE:
				triggerMalfunctionLed();
				stopDataAcquisition();
//				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
//				bsp_LaserPulse(7);

//				taskEEInit();

				/* Read the next user command */
				xQueueSend(queueReadCommand, &g_systemState.readcommand, portMAX_DELAY);
				break;

			/* A unknown command is received */
			case ErrUC_UnknownCommand:
				sprintf(str_buffer, "%d unknown command", 11 + event.param.error_level);
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
				/* Stop the data acquisition */
				stopDataAcquisition();

				/* Trigger the error LED */
				triggerMalfunctionLed();
				sendMessage(MSG_TYPE_STATE, "engine driver malfunction");
			break;

			/* Engine controller timeout */
			case Malf_Engine:
				/* Stop the data acquisition */
				stopDataAcquisition();

				/* Trigger the error LED */
				triggerMalfunctionLed();
				sendMessage(MSG_TYPE_STATE, "eingine is blocked");
				break;

			/* Laser overcurrent was detected */
			case Malf_LaserDriver:
				/* Stop the data acquisition */
				stopDataAcquisition();

				/* Trigger the error LED */
				triggerMalfunctionLed();
				sendMessage(MSG_TYPE_STATE, "laser driver malfunction");
				break;

			/* Quadrature encoder malfunction was detected */
			case Malf_QuadEnc:
				/* Trigger the error LED */
				triggerMalfunctionLed();
				sendMessage(MSG_TYPE_STATE, "quadrature encoder malfunction");
				break;

			/* TDC stat register has an unexpected value */
			case Malf_Tdc:
				/* Check the type of error */
				hits_error = 0;
				if (event.param.gp22_stat & 0xE000) {
					sendMessage(MSG_TYPE_STATE, "tdc eeprom malfunction");
					/* Trigger the error LED */
					triggerMalfunctionLed();
				}
				if (event.param.gp22_stat & 0x1800) {
					sendMessage(MSG_TYPE_STATE, "tdc temperature sensor malfunction");
					/* Trigger the error LED */
					triggerMalfunctionLed();
				}
				if (event.param.gp22_stat & 0x0400) {
					sendMessage(MSG_TYPE_STATE, "tdc precounter timeout");
					/* Trigger the error LED */
					triggerMalfunctionLed();
				}
				/* number of hits channel 2 */
				tdc_hits = (event.param.gp22_stat & 0x01C0) >> 6;
				if (tdc_hits > 1) {
					hits_error = 1;
					//sendMessage(MSG_TYPE_STATE, "receiver malfunction");
				}
				/* number of hits channel 1 */
				tdc_hits = (event.param.gp22_stat & 0x0038) >> 3;
				if (tdc_hits > 1) {
					hits_error = 1;
					sendMessage(MSG_TYPE_STATE, "monitor malfunction");
					/* Trigger the error LED */
					triggerMalfunctionLed();
				}
				if (tdc_hits == 0) {
					hits_error = 1;
					sendMessage(MSG_TYPE_STATE, "laser diode malfunction");
					/* Trigger the error LED */
					triggerMalfunctionLed();
				}
				if (hits_error == 0 && event.param.gp22_stat & 0x0200) {
					hits_error = 1;
					sendMessage(MSG_TYPE_STATE, "tdc timeout");
					/* Trigger the error LED */
					triggerMalfunctionLed();
				}

				break;

			/* Serial interface timeout occurs */
			case Marf_Serial:
				/* Sets the LED forever */
				bsp_LedSetOn(LED_MALFUNCTION);
				/* Error message is not possible due the malfunction in the gatekeeper */

				/* Disable all interrupts */
				__disable_irq();
				/* ... and hang on */
				for (;;) {

				}
				break;

			/* No space available in memory pool */
			case Fault_MemoryPool:
				/* Stop the data acquisition */
				stopDataAcquisition();

				/* Trigger the error LED */
				triggerMalfunctionLed();
				sendMessage(MSG_TYPE_STATE, "no space available in memory pool");
				break;

			/* Not allowed pointer to raw data memory */
			case Fault_MemoryPoolPtr:
				/* Stop the data acquisition */
				stopDataAcquisition();

				/* Trigger the error LED */
				triggerMalfunctionLed();
				sendMessage(MSG_TYPE_STATE, "internal timing malfunction");
				break;

			/* Command error */
			default:
				triggerMalfunctionLed();
				sendMessage(MSG_TYPE_STATE, "unknown controller command");
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
	bsp_LedSetOn(LED_MALFUNCTION);
}

/**
 * \brief	Stops the data acquisition after a malfunction.
 */
void stopDataAcquisition(void) {
	dataacquisition_t data_acquisition_config;

	if (g_systemState.state == MODE_DATA) {
		/* Stop the data acquisition */
		data_acquisition_config.state = DATA_ACQUISITION_DISABLE;
		data_acquisition_config.param.engine_sleep = g_systemState.engine_sleep;
		xQueueSend(queueDataAcquisition, &data_acquisition_config, portMAX_DELAY);

		/* Change the state */
		g_systemState.state = MODE_CMD;

		/* Reset the LED */
		bsp_LedSetOff(LED_LASER_OPERATION);

		/* Send the new state */
		sendMessage(MSG_TYPE_STATE, "cmd");
	}
}


/**
 * @}
 */

/**
 * @}
 */
