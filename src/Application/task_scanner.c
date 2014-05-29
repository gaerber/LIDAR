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

/* RTOS */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Application */
#include "task_scanner.h"

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
 */
void taskScanner(void* pvParameters) {
	TickType_t xLastWakeTime;

	uint32_t current_azimuth;
	uint32_t last_azimuth = 0;

	int32_t set_point;
	int32_t process_variable;

	// setpoint (sp) = Sollwert
	// measured process variable (PV) = Istwert

	/* Initialize the xLastWakeTime variable with the current time */
	xLastWakeTime = xTaskGetTickCount();

	/* Loop forever */
	for (;;) {
		/* Wait for the next cycle */
		vTaskDelayUntil(&xLastWakeTime, 1);

		/* Get the current azimuth */
		//BUG: wert wird immer verändert!
		bsp_QuadencGet(&current_azimuth);



		/* Calculate the current speed */
		process_variable = current_azimuth - last_azimuth;
		if (process_variable < 0) {
			process_variable += BSP_QUADENC_INC_PER_TURN;
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
