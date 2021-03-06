/**
 * \file		task_scanner.c
 * \brief		Controls the rotation of the mirror.
 * \date		2014-05-28
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	app
 * @{
 *
 * \addtogroup	task_scanner
 * @{
 */

#include <stdint.h>
#include <stdlib.h>

/* RTOS */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Application */
#include "task_scanner.h"
#include "task_controller.h"

/* BSP */
#include "bsp_engine.h"
#include "bsp_quadenc.h"

/*
 * ----------------------------------------------------------------------------
 * Private functions prototypes
 * ----------------------------------------------------------------------------
 */
void taskScanner(void* pvParameters);


/*
 * ----------------------------------------------------------------------------
 * Task synchronization
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Command interpreter task handle.
 */
TaskHandle_t taskScannerHandle;

/**
 * \brief	Queue to set a new setpiont value of the rotation speed.
 */
QueueHandle_t queueSpeed;


/*
 * ----------------------------------------------------------------------------
 * Implementation
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Scanner task initialization. Generates the task and initialize the hardware.
 */
void taskScannerInit(void) {

	/* Initialize the engine */
	bsp_EngineInit();

	/* Initialize the quadrature encoder */
	bsp_QuadencInit();

	/* Generate the task */
	xTaskCreate(taskScanner, TASK_SCANNER_NAME, TASK_SCANNER_STACKSIZE,
			NULL, TASK_SCANNER_PRIORITY, &taskScannerHandle);

	/* Generate the queue */
	queueSpeed = xQueueCreate(Q_SPEED_LENGTH, sizeof(speed_t));
}


/**
 * \brief	Scanner Task. Implementation of the scanner task with his own loop.
 * \param[in]	pvParameters task parameters. Not used.
 */
void taskScanner(void* pvParameters) {
	TickType_t xLastWakeTime;

	uint32_t current_azimuth = 0;
	uint32_t last_azimuth = 0;

	int32_t set_point = 1000;
	int32_t process_variable;
	int32_t e;
	int32_t e_sum = 0;
	int32_t controlling_element = 0;

	int32_t timeout;
	event_t event;

	/* Loop forever */
	for (;;) {
		/* Wait until the engine has to start */
		xQueueReceive(queueSpeed, &set_point, portMAX_DELAY);

		/* Enable the engine */
		bsp_EngineSpeed(0);
		bsp_EngineEnalble();

		/* Set the tmeout */
		timeout = ENGINE_RISE_TIME;

		/* Initialize the xLastWakeTime variable with the current time */
		xLastWakeTime = xTaskGetTickCount();

		/* Controller circuit of the engine */
		while (set_point != 0) {
			/* Wait for the next cycle */
			vTaskDelayUntil(&xLastWakeTime, ENGINE_CONTROLER_TA);

			/* Get the current azimuth */
			bsp_QuadencGet(&current_azimuth);

			/* Get the new set point value if there is one */
			xQueueReceive(queueSpeed, &set_point, 0);

			/* Calculate the current speed */
			process_variable = current_azimuth - last_azimuth;
			if (abs(process_variable) > BSP_QUADENC_INC_PER_TURN / 2) {
				/* Consider the restoring of the index */
				if (process_variable < 0) {
					process_variable += BSP_QUADENC_INC_PER_TURN;
				}
				else {
					process_variable -= BSP_QUADENC_INC_PER_TURN;
				}
			}
			last_azimuth = current_azimuth;

			/* Calculate the difference */
			e = set_point - process_variable;

			/* Integrator */
			e_sum = e_sum + e;

			/* PI controller */
			controlling_element  = ENGINE_CONTROLER_KP * e + ENGINE_CONTROLER_KI * ENGINE_CONTROLER_TA * e_sum;

			/* Limit the controlling element */
			if (controlling_element > ENGINE_MAX_POWER) {
				controlling_element = ENGINE_MAX_POWER;
				/* Anti windup */
				e_sum = e_sum - e;
				/* Check blocking engine */
				if (timeout-- == 0) {
					/* Sends the failure to the controller */
					event.event = Malf_Engine;
					xQueueSend(queueEvent, &event, portMAX_DELAY);
				}
			}
			else if (controlling_element < (-1 * ENGINE_MAX_POWER)) {
				controlling_element = -1 * ENGINE_MAX_POWER;
				/* Anti windup */
				e_sum = e_sum - e;
				/* Check blocking engine */
				if (timeout-- == 0) {
					/* Sends the failure to the controller */
					event.event = Malf_Engine;
					xQueueSend(queueEvent, &event, portMAX_DELAY);
				}
			}
			else {
				timeout = ENGINE_RISE_TIME;
			}

			/* Sets the new controlling element */
			bsp_EngineSpeed(controlling_element);
		} /* End of controller loop */

		/* Suspend the engine */
		bsp_EngineSpeed(0);
		bsp_EngineDisable();
	}

	/* Never reach this point */
}


/**
 * @}
 */

/**
 * @}
 */
