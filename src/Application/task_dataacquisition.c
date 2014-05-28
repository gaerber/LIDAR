/**
 * \file		task_dataacquisition.c
 * \brief		The data acquisition contains the azimuth and the distance.
 * \date		2014-05-28
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	app
 * @{
 *
 * \addtogroup	task_dataacquisition
 * @{
 */

#include <stdint.h>

/* RTOS */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Application */
#include "task_dataacquisition.h"


/*
 * ----------------------------------------------------------------------------
 * Private functions prototypes
 * ----------------------------------------------------------------------------
 */
void taskDataAcquisition(void* pvParameters);


/*
 * ----------------------------------------------------------------------------
 * Task synchronization
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Data acquisition task handle.
 */
TaskHandle_t taskDataAcquisitionHandle;


/*
 * ----------------------------------------------------------------------------
 * Implementation
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Data acquisition task initialization. Generates the task and initialize the hardware.
 */
void taskDataAcquisitionInit(void) {

	/* TDC initialization */

	/* Quadrature encoder initialization */

	/* Generate the task */
	xTaskCreate(taskDataAcquisition, TASK_DATAACQUISITION_NAME, TASK_DATAACQUISITION_STACKSIZE,
			NULL, TASK_DATAACQUISITION_PRIORITY, &taskDataAcquisitionHandle);
}


/**
 * \brief	Data acquisition Task. Implementation of the Data acquisition task with his own loop.
 */
void taskDataAcquisition(void* pvParameters) {

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
