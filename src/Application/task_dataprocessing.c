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
#include "task_gatekeeper.h"

/* BSP */
#include "bsp_quadenc.h"
#include "bsp_gp22.h"

/* Utility */
#include "data_encode.h"
#include "incs_azimuth.h"


/*
 * ----------------------------------------------------------------------------
 * Private functions prototypes
 * ----------------------------------------------------------------------------
 */
void taskDataProcessing(void* pvParameters);
inline void dataEncode(int16_t azimuth, int16_t distance, char *base64);


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
 * \param[in]	pvParameters task parameters. Not used.
 */
void taskDataProcessing(void* pvParameters) {
	rawdata_t *raw_data;

	uint32_t current_cal_resonator = 0;
	double cal_resonator_factor = 1.0;

	uint32_t i;
	double mean_value;

	int16_t azimuth;

	char room_map_point[DATA_MESSAGE_STRING_LENGTH];

	double time, distance;

	/* Loop forever */
	for (;;) {
		/* Get the new raw data from data acquisition */
		if (xQueueReceive(queueRawDataPtr, &raw_data, portMAX_DELAY) == pdTRUE) {
			/* Calculate the new calibration factor if necessary */
			if (raw_data->cal_resonator != current_cal_resonator) {
				cal_resonator_factor = (1.0 / BSP_GP22_RESONATOR) / (2.0 * (raw_data->cal_resonator / (double) 0xFFFF));
				current_cal_resonator = raw_data->cal_resonator;
			}

			/* Calculate the mean value */
			mean_value = 0.0;
			for (i=0; i<raw_data->raw_ctr; i++) {
				mean_value += raw_data->raw[i];
			}
			mean_value = mean_value / raw_data->raw_ctr;

			/* Calculate the azimuth [tenth degree] */
			azimuth = increments2tenthdegree(raw_data->increments);

			/* Give the memory block */
			eMemGiveBlock(&memRawData, raw_data);

			/* Calculate the distance */
			time = (mean_value / (double) 0xFFFF) * cal_resonator_factor * (1.0 / BSP_GP22_HS_CRYSTAL);
			distance = VERILOG_OF_LIGHT / 2.0 * time;

			/* Encode the data of the point of the room map */
			dataEncode(azimuth, distance, room_map_point);

			/* Send the calculated result to the gatekeeper task */
			xQueueSend(queueMessageData, room_map_point, portMAX_DELAY);
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
