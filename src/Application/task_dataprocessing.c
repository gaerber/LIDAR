/**
 * \file		task_dataprocessing.c
 * \brief		Data processing task.
 * \date		2014-06-02
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	app
 * @{
 *
 * \addtogroup	task_dataprocessing
 * @{
 */

#include <stdint.h>

/* RTOS */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "memPoolService.h"

/* Application */
#include "task_dataprocessing.h"

/* BSP */
#include "bsp_quadenc.h"


/*
 * ----------------------------------------------------------------------------
 * Private functions prototypes
 * ----------------------------------------------------------------------------
 */
void taskDataProcessing(void* pvParameters);


/*
 * ----------------------------------------------------------------------------
 * Task synchronization
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Data processing handle.
 */
TaskHandle_t taskDataProcessingHandle;

/**
 * \brief	Queue with the pointers to the storage address of the raw data.
 * 			Each pointer must show on a valid address from the memory pool
 * 			memRawData.
 */
QueueHandle_t queueRawDataPtr;

/**
 * \brief	Memory pool with the raw data.
 */
MemPoolManager memRawData;

/**
 * \brief	Storage of the memory pool.
 */
rawdata_t g_memRawDataStorage[Q_RAWDATA_LENGTH];


/*
 * ----------------------------------------------------------------------------
 * Implementation
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Controller task initialization. Generates the task and initialize the hardware.
 */
void taskDataProcessingInit(void) {

	/* Generate the task */
	xTaskCreate(taskDataProcessing, TASK_DATAPROC_NAME, TASK_DATAPROC_STACKSIZE,
			NULL, TASK_DATAPROC_PRIORITY, &taskDataProcessingHandle);

	/* Generate the memory pool */
	eMemCreateMemoryPool(&memRawData, g_memRawDataStorage,
			sizeof(rawdata_t), Q_RAWDATA_LENGTH, "Raw Data");

	/* Generate the queue */
	queueRawDataPtr = xQueueCreate(Q_RAWDATA_LENGTH, sizeof(rawdata_t *));
}

/**
 * \brief	Controller Task. Implementation of the controller task with his own loop.
 */
void taskDataProcessing(void* pvParameters) {
	rawdata_t *raw_data;

	/* Loop forever */
	for (;;) {
		/* Get the new raw data from data acquisition */
		if (xQueueReceive(queueRawDataPtr, &raw_data, portMAX_DELAY) == pdTRUE) {
			/* Make some calculations */

			/* Give the memory block */
			eMemGiveBlock(&memRawData, raw_data);

			/* Send the calculated result to the gatekeeper task */

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
