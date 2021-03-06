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
#include "memPoolService.h"
#include "timers.h"

/* Application */
#include "task_dataacquisition.h"
#include "task_controller.h"
#include "task_scanner.h"
#include "task_dataprocessing.h"


/* BSP */
#include "bsp_laser.h"
#include "bsp_gp22.h"
#include "bsp_quadenc.h"
#include "bsp_engine.h"

/* Utility */
#include "incs_azimuth.h"


/*
 * ----------------------------------------------------------------------------
 * Private data types
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Structure of all necessary configuration of the data acquisition.
 */
typedef struct {
	uint32_t azimuth_left;		/*!< Left azimuth of the scanning area. */
	uint32_t azimuth_right;		/*!< Right azimuth of the scanning area. */
	uint32_t azimuth_res;		/*!< Resolution between two measurement points. */
	uint32_t laser_pulses;		/*!< Number of laser pulses each measurement point. */
	uint8_t enable;				/*!< State of the data acquisition. TRUE if enabled. */
} acquisitionconfigs_t;


/*
 * ----------------------------------------------------------------------------
 * Private functions prototypes
 * ----------------------------------------------------------------------------
 */
void taskDataAcquisition(void* pvParameters);
void azimuthTDCCalibrationHandler(uint32_t azimuth);
void tdcHighSpeedCalibrationHandler(void);
void azimuthMeasurementHandler(uint32_t azimuth);
void tdcMeasurementHandler(void);
void laserEndSequenceHandler(void);

void engineStandByCallback(TimerHandle_t xTimer);
void DataAcquisitionStartCallback(TimerHandle_t xTimer);


/*
 * ----------------------------------------------------------------------------
 * Task synchronization
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Data acquisition task handle.
 */
TaskHandle_t taskDataAcquisitionHandle;

/**
 * \brief	Data with the settings of the data acquisition.
 */
QueueHandle_t queueDataAcquisition;


/*
 * -----------------------------------------------------------------------
 * Private variables
 * -----------------------------------------------------------------------
 */

/**
 * \brief	All configuration of the data acquisition.
 */
static acquisitionconfigs_t g_configs;

/**
 * \brief	Pointer to the storage of the raw data. Get the space form a
 * 			memory pool.
 */
rawdata_t *g_rawDataPtr;

/**
 * \brief	Calibration raw value. It is updated very turn.
 */
uint32_t g_rawCalibrationData;

/**
 * \brief	Software timer handler for the engine sleep feature.
 */
TimerHandle_t timerEngineSleep;

/**
 * \brief	Software timer handler to start the data acquisition after reaching the engine speed.
 */
TimerHandle_t timerDataAcquisitionStart;


/*
 * ----------------------------------------------------------------------------
 * Timer callback functions
 * ----------------------------------------------------------------------------
 */

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
 * \brief	Timer callback function to start the data acquisition after the setting
 * 			time of the engine controller.
 * \param[in]	xTimer The identifier that is assigned to the timer being called.
 * 				Not used.
 */
void DataAcquisitionStartCallback(TimerHandle_t xTimer) {
	/* Starts the data acquisition */
	g_configs.enable = 1;

	/* Starts the data acquisition with a calibration measurement of the
	 * high speed clock from the TDC */
	bsp_QuadencPosCallback(azimuthTDCCalibrationHandler);
	bsp_QuadencSetCapture(tenthdegree2increments(DA_AZIMUTH_CAL_RES));
}


/*
 * ----------------------------------------------------------------------------
 * Hook functions
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Quadrature encoder hook function. It is called after a increment
 * 			failure from a ISR function.
 */
void bsp_QuadencRoterrorHook(void) {
	event_t event;
	BaseType_t xTaskWoken = pdFALSE;

	/* Sends the failure to the controller */
	event.event = Malf_QuadEnc;
	xQueueSendFromISR(queueEvent, &event, &xTaskWoken);

	/* Check if a higher prior task is woken up */
	portEND_SWITCHING_ISR(xTaskWoken);
}


/*
 * ----------------------------------------------------------------------------
 * Implementation
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Data acquisition task initialization. Generates the task and initialize the hardware.
 */
void taskDataAcquisitionInit(void) {
	/* Initialize the laser pulse generator */
	bsp_LaserInit();
	bsp_LaserSequenceCalback(laserEndSequenceHandler);

	/* Initialize the TDC-GP22 */
	bsp_GP22Init();

	/* Initialize the quadrature encoder */
	bsp_QuadencInit();

	/* Disable the data acquisition */
	g_configs.enable = 0;

	/* Reset the static variables */
	g_rawDataPtr = NULL;
	g_rawCalibrationData = 0;

	/* Generate the task */
	xTaskCreate(taskDataAcquisition, TASK_DATAACQUISITION_NAME, TASK_DATAACQUISITION_STACKSIZE,
			NULL, TASK_DATAACQUISITION_PRIORITY, &taskDataAcquisitionHandle);

	/* Generate the queue */
	queueDataAcquisition = xQueueCreate(Q_DATAACQUISITION_LENGTH, sizeof(dataacquisition_t));

	/* Generate the timer */
	timerEngineSleep = xTimerCreate("Engine sleep", 3000/portTICK_PERIOD_MS,
			pdFALSE, NULL, engineStandByCallback);

	/* Generate the timer */
	timerDataAcquisitionStart = xTimerCreate("Data Acquisition", ENGINE_SETTING_TIME/portTICK_PERIOD_MS,
			pdFALSE, NULL, DataAcquisitionStartCallback);
}


/**
 * \brief	Data acquisition Task. Implementation of the Data acquisition task with his own loop.
 * \param[in]	pvParameters task parameters. Not used.
 */
void taskDataAcquisition(void* pvParameters) {
	dataacquisition_t settings;
	speed_t engine_speed;

	event_t event;

	uint8_t laser_flag;
	uint8_t laser_flag_last = 1;
	uint8_t engine_flag;
	uint8_t engine_flag_last = 1;

	/* Loop forever */
	for (;;) {
		/* Wait for new configuration settings. */
		if (xQueueReceive(queueDataAcquisition, &settings, 100) == pdTRUE) {
			/* Check the new state */
			if (settings.state == DATA_ACQUISITION_ENABLE) {
				/* Starts the data acquisition */
				engine_speed = settings.param.scan.rate * (BSP_QUADENC_INC_PER_TURN+1) / (1000*ENGINE_CONTROLER_TA);

				/* Starts the engine */
				xTimerStop(timerEngineSleep, portMAX_DELAY);
				xQueueSend(queueSpeed, &engine_speed, portMAX_DELAY);

				/* Calculate the settings */
				g_configs.azimuth_left = tenthdegree2increments(settings.param.scan.bndry_left);
				g_configs.azimuth_right = tenthdegree2increments(settings.param.scan.bndry_right);
				g_configs.azimuth_res = tenthdegree2increments_Relative(settings.param.scan.step);
				g_configs.laser_pulses =  DA_LASERPULSE / settings.param.scan.rate;

				/* Check if the engine is running */
				if (xTimerIsTimerActive(timerEngineSleep) != pdFALSE) {
					/* Stops the engine delay sleep timer */
					xTimerStop(timerEngineSleep, portMAX_DELAY);
					/* Starts the data acquisition direct */
					DataAcquisitionStartCallback(NULL);
				}
				else {
					/* Starts after a small time delay */
					xTimerStart(timerDataAcquisitionStart, portMAX_DELAY);
				}
			}
			else {
				/* Stops the data acquisition */
				xTimerStop(timerDataAcquisitionStart, portMAX_DELAY);
				g_configs.enable = 0;

				/* Stop the engine after a given time delay */
				if (settings.param.engine_sleep > 0) {
					xTimerChangePeriod(timerEngineSleep, settings.param.engine_sleep, portMAX_DELAY);
					xTimerStart(timerEngineSleep, portMAX_DELAY);
				}
				else {
					/* Stop the engine now */
					engine_speed = 0;
					xQueueSend(queueSpeed, &engine_speed, portMAX_DELAY);
				}
			}
		}

		/* Make the system check of the data acquisition module */

		/* --- Laser driver error flag ------------------------ */
		laser_flag = bsp_LaserOvercurrent();

		/* Check if a overcurrent is ocured */
		if (!laser_flag && laser_flag_last) {
			event.event = Malf_LaserDriver;
			if (xQueueSend(queueEvent, &event, 0) == pdTRUE) {
				laser_flag_last = laser_flag;
			}
			/* If queue is full, wait for an other check cycle */
		}
		else {
			laser_flag_last = laser_flag;
		}

		/* --- Engine driver error flag ----------------------- */
		engine_flag = bsp_EngineAlert();

		/* Check if a overcurrent is ocured */
		if (!engine_flag && engine_flag_last) {
			event.event = Malf_EngineDriver;
			if (xQueueSend(queueEvent, &event, 0) == pdTRUE) {
				engine_flag_last = engine_flag;
			}
			/* If queue is full, wait for an other check cycle */
		}
		else {
			engine_flag_last = engine_flag;
		}
	}

	/* Never reach this point */
}


/*
 * ----------------------------------------------------------------------------
 * TDC high speed clock calibration
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Azimuth interrupt handler to calibrate the TDC high speed clock.
 * \param[in]	azimuth is the current azimuth, which called the interrupt.
 */
void azimuthTDCCalibrationHandler(uint32_t azimuth) {
	uint32_t reg;

	/* Check if it is enabled */
	if (g_configs.enable) {
		/* Configure the next step: Propagation delay calibration */
		bsp_QuadencPosCallback(azimuthMeasurementHandler);
		bsp_QuadencSetCapture(tenthdegree2increments(DA_AZIMUTH_CAL_DIST));

#if (BSP_GP22_REG0 & (1<<13))
		/* Disable the automatic calibration calculation on the TDC */
		reg = BSP_GP22_REG0 & (~(1<<13));
		bsp_GP22RegWrite(GP22_WR_REG_0, reg);
#endif

#if (BSP_GP22_REG1 & (1<<23))
		/* Disable the fast init feature */
		reg = BSP_GP22_REG1 & (~(1<<23));
		bsp_GP22RegWrite(GP22_WR_REG_1, reg);
#endif

#if (!((BSP_GP22_REG2 & (1<<31)) && (BSP_GP22_REG2 & (1<<29))))
		/* Set the TDC interrupt source to TDC timeout and ALU interrupt */
		reg = BSP_GP22_REG2 | (1<<31) | (1<<29);
		bsp_GP22RegWrite(GP22_WR_REG_2, reg);
#endif

		/* Starts a calibration measurement for the high speed clock */
		bsp_GP22IntCallback(tdcHighSpeedCalibrationHandler);
		bsp_GP22SendOpcode(GP22_OP_Init);
		bsp_GP22SendOpcode(GP22_OP_Start_Cal_Resonator);
	}
	else {
		/* Data acquisition disable */
		bsp_QuadencPosCallback(NULL);
	}
}

/**
 * \brief	TDC interrupt handler, called after a high speed clock calibration
 * 			measurement.
 */
void tdcHighSpeedCalibrationHandler(void) {
	/* Read the calibration value */
	bsp_GP22RegRead(GP22_RD_RES_0, &g_rawCalibrationData, 4);

	/* Reset the configuration */
#if (BSP_GP22_REG0 & (1<<13))
	bsp_GP22RegWrite(GP22_WR_REG_0, BSP_GP22_REG0);
#endif

#if (BSP_GP22_REG1 & (1<<23))
	bsp_GP22RegWrite(GP22_WR_REG_1, BSP_GP22_REG1);
#endif

#if (!((BSP_GP22_REG2 & (1<<31)) && (BSP_GP22_REG2 & (1<<29))))
	bsp_GP22RegWrite(GP22_WR_REG_2, BSP_GP22_REG2);
#endif

	/* Make the TDC ready for the measurements (EN_FAST_INIT) */
	bsp_GP22SendOpcode(GP22_OP_Init);
}


/*
 * ----------------------------------------------------------------------------
 * Propagation delay measurement
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Azimuth interrupt handler to a propagation delay measurement.
 * \param[in]	azimuth is the current azimuth, which called the interrupt.
 */
void azimuthMeasurementHandler(uint32_t azimuth) {
	uint32_t next_azimuth;
	event_t error_event;
	BaseType_t xTaskWoken = pdFALSE;

	/* Check if it is enabled */
	if (g_configs.enable) {
		/* Configure the next azimuth interrupt */
		next_azimuth = azimuth + g_configs.azimuth_res;
		if (azimuth == tenthdegree2increments(DA_AZIMUTH_CAL_DIST)) {
			/* First measurement point */
			bsp_QuadencPosCallback(azimuthMeasurementHandler);
			bsp_QuadencSetCapture(g_configs.azimuth_left);
		}
		else if (next_azimuth <= g_configs.azimuth_right) {
			/* Set the next azimuth value */
			bsp_QuadencSetCapture(next_azimuth);
		}
		else {
			/* Scan completed -> restart with calibration */
			bsp_QuadencPosCallback(azimuthTDCCalibrationHandler);
			bsp_QuadencSetCapture(tenthdegree2increments(DA_AZIMUTH_CAL_RES));
		}

		/* The last sequence must be done */
		if (g_rawDataPtr == NULL) {
			/* Get a memory block for the raw data */
			if (eMemTakeBlockFromISR(&memRawData, (void**)&g_rawDataPtr, &xTaskWoken) == MEM_NO_ERROR) {
				/* Set the default values */
				g_rawDataPtr->cal_resonator = g_rawCalibrationData;
				g_rawDataPtr->increments = azimuth;
				g_rawDataPtr->expected_points = g_configs.laser_pulses;
				g_rawDataPtr->raw_ctr = 0;

				/* Set the TDC callback function */
				bsp_GP22IntCallback(tdcMeasurementHandler);

				/* Starts a measurement sequence */
				bsp_LaserPulse(g_configs.laser_pulses);
			}
			else {
				/* Send an error message to the controller */
				error_event.event = Fault_MemoryPool;
				xQueueSendFromISR(queueEvent, &error_event, &xTaskWoken);
			}
		}
		else {
			/* Send an error message to the controller */
			error_event.event = Fault_Timing;
			xQueueSendFromISR(queueEvent, &error_event, &xTaskWoken);
		}
	}
	else {
		/* Data acquisition disable */
		bsp_QuadencPosCallback(NULL);
	}

	/* Check if a higher prior task is woken up */
	portEND_SWITCHING_ISR(xTaskWoken);
}

/**
 * \brief	TDC interrupt handler, called after the propagation delay measurement.
 */
void tdcMeasurementHandler(void) {
	uint32_t result;
	event_t error_event;
	BaseType_t xTaskWoken = pdFALSE;

	/* Check the pointer */
	if (g_rawDataPtr != NULL) {
		/* Read the calibration value */
		bsp_GP22RegRead(GP22_RD_RES_0, &result, 4);
		/* Safe the raw data */
		g_rawDataPtr->raw[g_rawDataPtr->raw_ctr++] = result;
	}
	else {
		/* Send error event */
		error_event.event = Fault_MemoryPoolPtr;
		xQueueSendFromISR(queueEvent, &error_event, &xTaskWoken);
	}

	/* Check if a higher prior task is woken up */
	portEND_SWITCHING_ISR(xTaskWoken);
}

/**
 * \brief	Laser interrupt handler. Its function will called at the end of a
 * 			laser pulse sequence.
 */
void laserEndSequenceHandler(void) {
	uint32_t stat;
	event_t error_event;
	BaseType_t xTaskWoken = pdFALSE;

	/* Check the pointer */
	if (g_rawDataPtr != NULL) {
		/* Check the received numbers */
		if (g_rawDataPtr->raw_ctr < g_configs.laser_pulses) {
			/* Not all pulses were successfully -> control sample */
			bsp_GP22RegRead(GP22_RD_STAT, &stat, 2);
			/* Stat is 0x0000 if the last sample was successful,
			 * Stat is 0x0208 if a timeout occurs due to missing reflection */
			if (!(stat == 0x0000 || (stat&0xFFF8) == 0x0208)) {
				/* Send an error event with the value of the state register */
				error_event.event = Malf_Tdc;
				error_event.param.gp22_stat = stat;
				xQueueSendFromISR(queueEvent, &error_event, &xTaskWoken);
			}
		}

		/* Send the raw data pointer to the data processing task */
		if (xQueueSendFromISR(queueRawDataPtr, &g_rawDataPtr, &xTaskWoken) == pdTRUE) {
			/* Reset pointer */
			g_rawDataPtr = NULL;
		}
		else {
			/* Send an error event with the value of the state register */
			error_event.event = Fault_MemoryPool;
			xQueueSendFromISR(queueEvent, &error_event, &xTaskWoken);
		}
	}
	else {
		/* Send error event */
		error_event.event = Fault_MemoryPoolPtr;
		xQueueSendFromISR(queueEvent, &error_event, &xTaskWoken);
	}

	/* Check if a higher prior task is woken up */
	portEND_SWITCHING_ISR(xTaskWoken);
}


/**
 * @}
 */

/**
 * @}
 */
