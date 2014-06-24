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
#include <string.h>

/* RTOS */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Application */
#include "task_comminterp.h"
#include "task_controller.h"
#include "task_gatekeeper.h"

/* BSP */
#include "bsp_serial.h"


/*
 * ----------------------------------------------------------------------------
 * Private functions prototypes
 * ----------------------------------------------------------------------------
 */
void taskCommInterp(void* pvParameters);
void* parseCommand(char **msg);
void* parseCommandSet(char **msg);
void* parseCommandSetComm(char **msg);
void* parseCommandSetScan(char **msg);
void* parseCommandSetEngine(char **msg);
void* parseCommandGet(char **msg);
uint8_t parseParamOnOff(char **msg, uint8_t param_end, uint8_t *param);
uint8_t parseParamNumber(char **msg, uint8_t param_end, int32_t *param);


/*
 * ----------------------------------------------------------------------------
 * Private data types
 * ----------------------------------------------------------------------------
 */
typedef void* (*msg_filter_t)(char **msg);


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

	/* Initialize the serial interface */
	bsp_SerialInit();

	/* Generate the task */
	xTaskCreate(taskCommInterp, TASK_COMMINTERP_NAME, TASK_COMMINTERP_STACKSIZE,
			NULL, TASK_COMMINTERP_PRIORITY, &taskCommInterpHandle);

	/* Generate the queue */
	queueReadCommand = xQueueCreate(Q_READCOMMAND_LENGTH, sizeof(readcommand_t));
}


/**
 * \brief	Command interpreter Task. Implementation of the command interpreter task with his own loop.
 * \param[in]	pvParameters task parameters. Not used.
 */
void taskCommInterp(void* pvParameters) {
	event_t resolved_command;
	readcommand_t read_command;

	char command[COMMAND_BUFFER_LENGTH];
	char *command_ptr;
	int8_t write_index;
	int8_t command_complete;

	uint32_t i;
	static const char frame_end[] = MSG_FRAME_END;

	msg_filter_t filter_function;
	uint8_t timeout;

	/* Loop forever */
	for (;;) {
		/* The controller gives the approval to read an user command */
		if (xQueueReceive(queueReadCommand, &read_command, portMAX_DELAY) == pdTRUE) {
			/* Reset the command */
			write_index = 0;
			command_complete = 0;

			/* Check if the echo is enabled */
			if (read_command) {
				/* Takes the mutual exclusion to write into the circular buffer */
				xSemaphoreTake(mutexTxCircBuf, portMAX_DELAY);

				/* Send the message type */
				while (!bsp_SerialCharPut(MSG_TYPE_ECHO)) {
					vTaskDelay(10/portTICK_RATE_MS);
				}
			}

			/* Reads the message until the frame */
			do {
				if (bsp_SerialCharGet(&(command[write_index]))) {
					/* Check the command end line */
					if (command[write_index] == '\r' || command[write_index] == '\n') {
						/* Command end detected */

						/* Check if characters were received */
						if (write_index > 0) {
							if (read_command) {
								/* Send the frame end */
								for (i=0; i<sizeof(frame_end)-1; i++) {
									while (!bsp_SerialCharPut(frame_end[i])) {
										/* No space available in the circular buffer */
										vTaskDelay(10/portTICK_PERIOD_MS);
									}
								}

								/* Release the mutual exclusion */
								xSemaphoreGive(mutexTxCircBuf);
							}

							/* Terminate the string command */
							command[write_index] = '\0';
							command_complete = 1;
						}
					}
					else {
						/* Character echo */
						if (read_command && (command[write_index] != '\b' || write_index > 0)) {
							/* Puts the echo */
							while (!bsp_SerialCharPut(command[write_index])) {
								vTaskDelay(10/portTICK_RATE_MS);
							}
						}

						/* Backspace implementation (big feature) */
						if (command[write_index] == '\b') {
							/* Set two characters back */
							if (write_index > 0) {
								write_index -= 2;
							}
							else {
								write_index = -1;
							}
						}

						/* Increment index if more space for character */
						if (write_index < COMMAND_BUFFER_LENGTH - 1) {
							write_index++;
						}
						else {
							/* Command overflow detected */
							write_index = 0;
							/* Release the mutual exclusion */
							if (read_command) {
								xSemaphoreGive(mutexTxCircBuf);
							}
							/* Send an error to the controller */
							resolved_command.event = ErrUC_LineOverflow;
							xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);

							/* Exit the reading loop */
							command_complete = -1;
						}
					}
				}
				else {
					/* No data in the circular buffer */
					vTaskDelay(10/portTICK_RATE_MS);
				}
			}
			while (command_complete == 0);

			/* Parse if a message frame was read correctly */
			if (command_complete == 1) {
				/* Parse the command */
				timeout = 5;
				command_ptr = command;
				filter_function = parseCommand;
				do {
					filter_function = (msg_filter_t)filter_function(&command_ptr);
				}
				while (filter_function != NULL && timeout > 0);
			}
		}
	}

	/* Never reach this point */
}


/**
 * \brief	Parse a user command on level 0.
 * \param[in,out]	msg address of the string pointer. It will be changed to
 * 					the last read position of the string.
 * \return	Pointer to the parse function from the next level. If the parse is
 * 			finished, the return value will be NULL.
 */
void* parseCommand(char **msg) {
	uint8_t success = 0;
	event_t resolved_command;
	msg_filter_t next_func;

	switch (**msg) {
		/* cmd */
		case 'c':
			if (strcmp(*msg, "cmd") == 0) {
				/* Send command to the controller */
				resolved_command.event = UC_Cmd;
				xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
				next_func = NULL;
				success = 1;
			}
			break;

		/* data */
		case 'd':
			if (strcmp(*msg, "data") == 0) {
				/* Send command to the controller */
				resolved_command.event = UC_Data;
				xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
				next_func = NULL;
				success = 1;
			}
			break;

		/* reboot */
		case 'r':
			if (strcmp(*msg, "reboot") == 0) {
				/* Send command to the controller */
				resolved_command.event = UC_Reboot;
				xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
				next_func = NULL;
				success = 1;
			}
			break;

		/* set */
		case 's':
			if (strncmp(*msg, "set ", 4) == 0) {
				/* Update the parameters */
				next_func = parseCommandSet;
				*msg += 4;
				success = 1;
			}
			break;

		/* get */
		case 'g':
			if (strncmp(*msg, "get ", 4) == 0) {
				/* Update the parameters */
				next_func = parseCommandGet;
				*msg += 4;
				success = 1;
			}
			break;

		/* ee */
		case 'e':
			if (strcmp(*msg, "ee") == 0) {
				/* Send command to the controller */
				resolved_command.event = UC_EE;
				xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
				next_func = NULL;
				success = 1;
			}
			break;
	}

	/* Check if the command was correct */
	if (success == 0) {
		/* Send the error message to the controller */
		resolved_command.event = ErrUC_UnknownCommand;
		resolved_command.param.error_level = 0;
		xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
		next_func = NULL;
	}

	return (void*)next_func;
}


/**
 * \brief	Parse a "set" user command on level 1.
 * \param[in,out]	msg address of the string pointer. It will be changed to
 * 					the last read position of the string.
 * \return	Pointer to the parse function from the next level. If the parse is
 * 			finished, the return value will be NULL.
 */
void* parseCommandSet(char **msg) {
	uint8_t success = 0;
	event_t resolved_command;
	msg_filter_t next_func;

	switch (**msg) {
	/* set comm */
	case 'c':
		if (strncmp(*msg, "comm ", 5) == 0) {
			/* Update the parameters */
			next_func = parseCommandSetComm;
			*msg += 5;
			success = 1;
		}
		break;

	/* set scan */
	case 's':
		if (strncmp(*msg, "scan ", 5) == 0) {
			/* Update the parameters */
			next_func = parseCommandSetScan;
			*msg += 5;
			success = 1;
		}
		break;

	/* set engine */
	case 'e':
		if (strncmp(*msg, "engine ", 7) == 0) {
			/* Update the parameters */
			next_func = parseCommandSetEngine;
			*msg += 7;
			success = 1;
		}
		break;
	}

	/* Check if the command was correct */
	if (success == 0) {
		/* Send the error message to the controller */
		resolved_command.event = ErrUC_UnknownCommand;
		resolved_command.param.error_level = 1;
		xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
		next_func = NULL;
	}

	return (void*)next_func;
}


/**
 * \brief	Parse a "set comm" user command on level 2. All parameters were read
 * 			and checked in this function, before the resolved command will be sent
 * 			to the controller task.
 * \param[in,out]	msg address of the string pointer. It will be changed to
 * 					the last read position of the string.
 * \return	Always NULL to stop the parsing algorithms.
 */
void* parseCommandSetComm(char **msg) {
	uint8_t success = 0;
	event_t resolved_command;

	switch (**msg) {
		/* set comm echo */
		case 'e':
			if (strncmp(*msg, "echo ", 5) == 0) {
				/* Check the user parameters */
				*msg += 5;
				if (parseParamOnOff(msg, 1, &(resolved_command.param.echo))) {
					resolved_command.event = UC_SetCommEcho;
					xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
				}
				success = 1;
			}
			break;

		/* set comm respmsg */
		case 'r':
			if (strncmp(*msg, "respmsg ", 8) == 0) {
				/* Check the user parameters */
				*msg += 8;
				if (parseParamOnOff(msg, 1, &(resolved_command.param.respmsg))) {
					resolved_command.event = UC_SetCommRespmsg;
					xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
				}
				success = 1;
			}
			break;
	}

	/* Check if the command was correct */
	if (success == 0) {
		/* Send the error message to the controller */
		resolved_command.event = ErrUC_UnknownCommand;
		resolved_command.param.error_level = 2;
		xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
	}

	/* The command must be resolved or an error occurs */
	return (void*)NULL;
}


/**
 * \brief	Parse a "set scan" user command on level 2. All parameters were read
 * 			and checked in this function, before the resolved command will be sent
 * 			to the controller task.
 * \param[in,out]	msg address of the string pointer. It will be changed to
 * 					the last read position of the string.
 * \return	Always NULL to stop the parsing algorithms.
 */
void* parseCommandSetScan(char **msg) {
	uint8_t success = 0;
	event_t resolved_command;
	int32_t number1, number2;

	switch (**msg) {
		/* set scan bndry */
		case 'b':
			if (strncmp(*msg, "bndry ", 6) == 0) {
				/* Check the user parameters */
				*msg += 6;
				if (parseParamNumber(msg, 0, &number1) && parseParamNumber(msg, 1, &number2)) {
					/* Check if the value were in bound */
					if (number1 >= DA_AZIMUTH_MIN && number1 <= number2 && number2 <= DA_AZIMUTH_MAX) {
						resolved_command.event = UC_SetScanBndry;
						resolved_command.param.azimuth_bndry.left = (int16_t) number1;
						resolved_command.param.azimuth_bndry.right = (int16_t) number2;
						xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
					}
					else {
						resolved_command.event = ErrUC_ArgOutOfBounds;
						xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
					}
				}
				success = 1;
			}
			break;

		/* set scan step */
		case 's':
			if (strncmp(*msg, "step ", 5) == 0) {
				/* Check the user parameters */
				*msg += 5;
				if (parseParamNumber(msg, 1, &number1)) {
					/* Check if the value were in bound */
					if (number1 >= 18 && number1 <= 3600) {
						resolved_command.event = UC_SetScanStep;
						resolved_command.param.azimuth_step = (int16_t) number1;
						xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
					}
					else {
						resolved_command.event = ErrUC_ArgOutOfBounds;
						xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
					}
				}
				success = 1;
			}
			break;

		/* set scan rate */
		case 'r':
			if (strncmp(*msg, "rate ", 5) == 0) {
				/* Check the user parameters */
				*msg += 5;
				if (parseParamNumber(msg, 1, &number1)) {
					/* Check if the value were in bound */
					if (number1 > 0 && number1 <= 10) {
						resolved_command.event = UC_SetScanRate;
						resolved_command.param.scan_rate = number1;
						xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
					}
					else {
						resolved_command.event = ErrUC_ArgOutOfBounds;
						xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
					}
				}
				success = 1;
			}
			break;
	}

	/* Check if the command was correct */
	if (success == 0) {
		/* Send the error message to the controller */
		resolved_command.event = ErrUC_UnknownCommand;
		resolved_command.param.error_level = 2;
		xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
	}

	/* The command must be resolved or an error occurs */
	return (void*)NULL;
}


/**
 * \brief	Parse a "set engine" user command on level 2. All parameters were read
 * 			and checked in this function, before the resolved command will be sent
 * 			to the controller task.
 * \param[in,out]	msg address of the string pointer. It will be changed to
 * 					the last read position of the string.
 * \return	Always NULL to stop the parsing algorithms.
 */
void* parseCommandSetEngine(char **msg) {
	uint8_t success = 0;
	event_t resolved_command;
	int32_t number;

	switch (**msg) {
		/* set engine sleep */
		case 's':
			if (strncmp(*msg, "sleep ", 6) == 0) {
				/* Check the user parameters */
				*msg += 6;
				if (parseParamNumber(msg, 1, &number)) {
					/* Check if the value were in bound */
					if (number >= 0 && number <= 5000) {
					resolved_command.event = UC_SetEngineSleep;
					resolved_command.param.engine_sleep = number;
					xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
				}
				else {
					resolved_command.event = ErrUC_ArgOutOfBounds;
					xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
				}
				}
				success = 1;
			}
			break;
	}

	/* Check if the command was correct */
	if (success == 0) {
		/* Send the error message to the controller */
		resolved_command.event = ErrUC_UnknownCommand;
		resolved_command.param.error_level = 2;
		xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
	}

	/* The command must be resolved or an error occurs */
	return (void*)NULL;
}


/**
 * \brief	Parse a "get" user command on level 1. The resolved command will be sent
 * 			to the controller task. This commands have no parameters.
 * \param[in,out]	msg address of the string pointer.
 * \return	Always NULL to stop the parsing algorithms.
 */
void* parseCommandGet(char **msg) {
	uint8_t success = 0;
	event_t resolved_command;
	msg_filter_t next_func;

	switch (**msg) {
		/* get all */
		case 'a':
			if (strcmp(*msg, "all") == 0) {
				/* Send command to the controller */
				resolved_command.event = UC_GetAll;
				xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
				next_func = NULL;
				success = 1;
			}
			break;

		/* get ver */
		case 'v':
			if (strcmp(*msg, "ver") == 0) {
				/* Send command to the controller */
				resolved_command.event = UC_GetVer;
				xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
				next_func = NULL;
				success = 1;
			}
			break;

		/* get comm */
		case 'c':
			if (strcmp(*msg, "comm") == 0) {
				/* Send command to the controller */
				resolved_command.event = UC_GetComm;
				xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
				next_func = NULL;
				success = 1;
			}
			break;

		/* get scan */
		case 's':
			if (strcmp(*msg, "scan") == 0) {
				/* Send command to the controller */
				resolved_command.event = UC_GetScan;
				xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
				next_func = NULL;
				success = 1;
			}
			break;

		/* get engine */
		case 'e':
			if (strcmp(*msg, "engine") == 0) {
				/* Send command to the controller */
				resolved_command.event = UC_GetEngine;
				xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
				next_func = NULL;
				success = 1;
			}
			break;
	}

	/* Check if the command was correct */
	if (success == 0) {
		/* Send the error message to the controller */
		resolved_command.event = ErrUC_UnknownCommand;
		resolved_command.param.error_level = 1;
		xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
		next_func = NULL;
	}

	/* The command must be resolved or an error occurs */
	return (void*)next_func;
}


/**
 * \brief	Parse a boolean parameter. The parameter must be "on" or "off". They are equal to TRUE and FALSE.
 * \param[in,out]	msg address of the string pointer. It will be changed to the last read position of the string.
 * \param[in]	param_end is set to TRUE if it is the last parameter. In this case '\0' must follow.
 * \param[out]	param is the storage address of the parsed boolean.
 * \return	Return TRUE if the parameter was read correct.
 */
uint8_t parseParamOnOff(char **msg, uint8_t param_end, uint8_t *param) {
	uint8_t success = 1;
	event_t resolved_command;

	/* Check if it is on ... */
	if (strncmp(*msg, "on", 2) == 0) {
		*param = 1;
		*msg += 2;
	}
	/* ... or it is off ... */
	else if (strncmp(*msg, "off", 3) == 0) {
		*param = 0;
		*msg += 3;
	}
	/* Only this two parameters are allowed */
	else {
		/* Send the error message to the controller */
		resolved_command.event = ErrUC_FaultArgType;
		xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
		success = 0;
	}

	/* Check the number of arguments */
	if (success && ((param_end && **msg != '\0') || (!param_end && **msg != ' '))) {
		resolved_command.event = ErrUC_TooFewArgs;
		xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
		success = 0;
	}

	return success;
}


/**
 * \brief	Parse a numeric parameter. The parameter must be a numeric value
 * 			without a decimal point. It could be negative.
 * \param[in,out]	msg address of the string pointer. It will be changed to the last read position of the string.
 * \param[in]	param_end is set to TRUE if it is the last parameter. In this case '\0' must follow.
 * \param[out]	param is the storage address of the parsed number.
 * \return	Return TRUE if the parameter was read correct.
 */
uint8_t parseParamNumber(char **msg, uint8_t param_end, int32_t *param) {
	uint8_t success = 1;
	event_t resolved_command;
	uint8_t digits = 0;
	uint8_t negative_sign = 0;

	/* Set 0 as start value */
	*param = 0;

	/* Checkt if there is a sign */
	if (**msg == '-') {
		negative_sign = 1;
		*msg += 1;
	}

	/* Reads the number (maximum 9 digits -> overflow is not possible!) */
	while (success && (**msg != ' ' && **msg != '\0') && digits++ < 9) {
		/* Check if it is a digit */
		if ('0' <= **msg && **msg <= '9') {
			*param = 10 * (*param) + ((**msg) - '0');
			*msg += 1;
		}
		else {
			/* Fault argument type */
			resolved_command.event = ErrUC_FaultArgType;
			xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
			success = 0;
		}
	}

	/* Overflow protection */
	if (digits > 9) {
		resolved_command.event = ErrUC_ArgOutOfBounds;
		xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
		success = 0;
	}

	/* Check the number of arguments */
	if (success && ((param_end && **msg != '\0') || (!param_end && **msg != ' '))) {
		resolved_command.event = ErrUC_TooFewArgs;
		xQueueSend(queueEvent, &resolved_command, portMAX_DELAY);
		success = 0;
	}
	else {
		*msg += 1;
	}

	/* Make the number negative if nescessary */
	if (negative_sign) {
		*param = -1 * (*param);
	}

	return success;
}


/**
 * @}
 */

/**
 * @}
 */
