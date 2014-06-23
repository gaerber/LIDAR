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
#include "task_controller.h"

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
uint32_t maxValue(uint32_t *data, uint32_t length);


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
	int16_t distance_mm;
	int16_t distance_offset_mm = 0;

	char room_map_point[DATA_MESSAGE_STRING_LENGTH];

	double propagation_delay, distance, distance_mm_double;

	/* Loop forever */
	for (;;) {
		/* Get the new raw data from data acquisition */
		if (xQueueReceive(queueRawDataPtr, &raw_data, portMAX_DELAY) == pdTRUE) {
			/* Calculate the new calibration factor if necessary */
			if (raw_data->cal_resonator != current_cal_resonator) {
				cal_resonator_factor = (BSP_GP22_RESONATOR_CYCLE / BSP_GP22_RESONATOR) / (1.0 / BSP_GP22_HS_CRYSTAL * raw_data->cal_resonator / (double) 0xFFFF);
				current_cal_resonator = raw_data->cal_resonator;
			}

			if (raw_data->raw_ctr > raw_data->expected_points / 2) {
				/* Calculate the mean value */
				mean_value = 0.0;
				for (i=0; i<raw_data->raw_ctr; i++) {
					mean_value += raw_data->raw[i];
				}
				mean_value += (raw_data->expected_points - raw_data->raw_ctr) * (1.5 * 39375);
				mean_value = mean_value / raw_data->expected_points;
			}
			else {
				/* Set the maximum value */
				mean_value = 0x7FFFFFFF;
			}

			/* Calculate the azimuth [tenth degree] */
			azimuth = increments2tenthdegree(raw_data->increments);

			/* Give the memory block */
			eMemGiveBlock(&memRawData, raw_data);

			/* Calculate the distance */
			propagation_delay = (mean_value / (double) 0xFFFF) * cal_resonator_factor * (1.0 / BSP_GP22_HS_CRYSTAL);
			distance = VERILOG_OF_LIGHT / 2.0 * propagation_delay;

			//DEMO
			distance_mm_double = 181.9186 * distance;

			/* Check a distance overflow */
			if (distance_mm_double > (double) 0xFFF) {
				/* Set the maximum value */
				distance_mm_double = (double) 0xFFF;
			}

			distance_mm = distance_mm_double;

			if (azimuth == DA_AZIMUTH_CAL_DIST) {
				/* Set the new calibration offset */
				distance_offset_mm = distance_mm - DA_DISTANCE_CAL;
			}
			else {
				if (distance_mm != 0xFFF) {
					distance_mm = distance_mm - distance_offset_mm;
				}

				/* Encode the data of the point of the room map */
				dataEncode(azimuth, distance_mm, room_map_point);

				/* Send the calculated result to the gatekeeper task */
				xQueueSend(queueMessageData, room_map_point, portMAX_DELAY);
			}

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
