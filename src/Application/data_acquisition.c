/**
 * \file		dataacquisition.c
 * \brief		The data acquisition contains the azimuth and the distance.
 * \date		2014-05-29
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	app
 * @{
 *
 * \addtogroup	task_controller
 * @{
 *
 * \addtogroup	dataacquisition
 * @{
 */

#include <stdint.h>

/* RTOS */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "memPoolService.h"

/* Application */
#include "data_acquisition.h"
#include "task_dataprocessing.h"

/* BSP */
#include "bsp_laser.h"
#include "bsp_gp22.h"
#include "bsp_quadenc.h"


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
} dataaquisition_t;


/*
 * ----------------------------------------------------------------------------
 * Private functions prototypes
 * ----------------------------------------------------------------------------
 */
void azimuthTDCCalibrationHandler(uint32_t azimuth);
void tdcHighSpeedCalibrationHandler(void);
void azimuthPDCalibrationHandler(uint32_t azimuth);
void tdcPropagationDelayCalibrationHandler(void);
void azimuthMeasurementHandler(uint32_t azimuth);
void tdcMeasurementHandler(void);

/*
 * -----------------------------------------------------------------------
 * Private variables
 * -----------------------------------------------------------------------
 */

/**
 * \brief	All configuration of the data acquisition.
 */
static dataaquisition_t g_settings;

/**
 * \brief	Pointer to the storage of the raw data. Get the space form a
 * 			memory pool.
 */
rawdata_t *g_rawDataPtr;

/**
 * \brief	Calibration raw value. It is updated very turn.
 */
uint32_t g_rawCalibrationData;


/*
 * ----------------------------------------------------------------------------
 * Implementation
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Initialize and configure the hardware for the data acquisition.
 */
void DataAcquisitionInit(void) {
	/* Initialize the laser pulse generator */
	bsp_LaserInit();

	/* Initialize the TDC-GP22 */
	bsp_GP22Init();

	/* Initialize the quadrature encoder */
	bsp_QuadencInit();

	/* Set the default settings */
	g_settings.azimuth_left = DA_AZIMUTH_MIN;
	g_settings.azimuth_right = DA_AZIMUTH_MAX;
	g_settings.azimuth_res = DA_AZIMUTH_RES;
	g_settings.laser_pulses = 25;
	g_settings.enable = 0;

	g_rawDataPtr = NULL;
	g_rawCalibrationData = 0;
}

/**
 * \brief	Starts the data acquisition in the given scanning area.
 * \param[in]	atzimuth_left
 */
void DataAcquisitionStart(uint32_t atzimuth_left, uint32_t azimuth_right,
		uint32_t azimuth_res, uint32_t laser_pulses) {
	/* Check the parameters */

	/* Save the new parameters */
	g_settings.azimuth_left = atzimuth_left;
	g_settings.azimuth_right = azimuth_right;
	g_settings.azimuth_res = azimuth_res;
	g_settings.laser_pulses = laser_pulses;
	g_settings.enable = 1;

	/* Starts the data acquisition with a calibration measurement of the
	 * high speed clock from the TDC */
	bsp_QuadencPosCallback(azimuthTDCCalibrationHandler);
	bsp_QuadencSetCapture(DA_AZIMUTH_CAL_RES);
}

/**
 * \brief	Stops the data acquisition.
 */
void DataAcquisitionStop(void) {
	g_settings.enable = 0;
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
	if (g_settings.enable) {
		/* Configure the next step: Propagation delay calibration */
		bsp_QuadencPosCallback(azimuthPDCalibrationHandler);
		bsp_QuadencSetCapture(DA_AZIMUTH_CAL_DIST);

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

	/* Make the TDC ready for a measurement */
	bsp_GP22SendOpcode(GP22_OP_Init);
}


/*
 * ----------------------------------------------------------------------------
 * Propagation delay calibration
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Azimuth interrupt handler to calibrate the propagation delay.
 * \param[in]	azimuth is the current azimuth, which called the interrupt.
 */
void azimuthPDCalibrationHandler(uint32_t azimuth) {
	portBASE_TYPE xTaskWoken = pdFALSE;

	/* Check if it is enabled */
	if (g_settings.enable) {
		/* Configure the next azimuth interrupt: First measurement point */
		bsp_QuadencPosCallback(azimuthMeasurementHandler);
		bsp_QuadencSetCapture(g_settings.azimuth_left);

		/* Get a memory block for the raw data */
		if (eMemTakeBlockFromISR(&memRawData, (void**)&g_rawDataPtr, &xTaskWoken) == MEM_NO_ERROR) {
			/* Set the default values */
			g_rawDataPtr->cal_resonator = g_rawCalibrationData;
			g_rawDataPtr->increments = azimuth;
			g_rawDataPtr->raw_ctr = 0;

			/* Set the TDC callback function */
			bsp_GP22IntCallback(tdcPropagationDelayCalibrationHandler);

			/* Starts a measurement sequence */
			bsp_LaserPulse(g_settings.laser_pulses);
		}
		else {
			/* Send an error message to the controller */

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
void tdcPropagationDelayCalibrationHandler(void) {
	uint32_t result;

	/* Read the state register */
	bsp_GP22RegRead(GP22_RD_STAT, &result, 2);

	/* Check the state */
	if ((result & 0xF8) == 0x48) {
		/* Read the calibration value */
		bsp_GP22RegRead(GP22_RD_RES_0, &result, 4);
		/* Safe the raw data */
		g_rawDataPtr->raw[g_rawDataPtr->raw_ctr++] = result;
	}
	else {
		/** @Todo: Error handling */
		//Direkt an Controller senden
	}

	/* Make the TDC ready for a measurement */
	bsp_GP22SendOpcode(GP22_OP_Init);
}


/*
 * ----------------------------------------------------------------------------
 * Normal distance measurement
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Azimuth interrupt handler to make a measurement of the room.
 * \param[in]	azimuth is the current azimuth, which called the interrupt.
 */
void azimuthMeasurementHandler(uint32_t azimuth) {
	uint32_t next_azimuth;
	portBASE_TYPE xTaskWoken = pdFALSE;

	/* Check if it is enabled */
	if (g_settings.enable) {
		/* Configure the next azimuth interrupt */
		next_azimuth = azimuth + g_settings.azimuth_res;
		if (next_azimuth <= g_settings.azimuth_right) {
			/* Set the next azimuth value */
			bsp_QuadencSetCapture(next_azimuth);
		}
		else {
			/* Scan completed -> restart with calibration */
			bsp_QuadencPosCallback(azimuthTDCCalibrationHandler);
			bsp_QuadencSetCapture(DA_AZIMUTH_CAL_RES);
		}

		/* Get a memory block for the raw data */
		if (eMemTakeBlockFromISR(&memRawData, (void**)&g_rawDataPtr, &xTaskWoken) == MEM_NO_ERROR) {
			/* Set the default values */
			g_rawDataPtr->cal_resonator = g_rawCalibrationData;
			g_rawDataPtr->increments = azimuth;
			g_rawDataPtr->raw_ctr = 0;

			/* Set the TDC callback function */
			bsp_GP22IntCallback(tdcMeasurementHandler);

			/* Starts the measurement */
			bsp_LaserPulse(g_settings.laser_pulses);
		}
		else {
			/* Send an error message to the controller */

		}
	}
	else {
		/* Data acquisition disable */
		bsp_QuadencPosCallback(NULL);
	}

	/* Make the TDC ready for a measurement */
	bsp_GP22SendOpcode(GP22_OP_Init);
}

/**
 * \brief	TDC interrupt handler, called after the propagation delay measurement.
 */
void tdcMeasurementHandler(void) {
	uint32_t result;

	/* Read the state register */
	bsp_GP22RegRead(GP22_RD_STAT, &result, 2);

	/* Check the state */
	if ((result & 0xF8) == 0x48) {
		/* Read the calibration value */
		bsp_GP22RegRead(GP22_RD_RES_0, &result, 4);
		/* Safe the raw data */
		g_rawDataPtr->raw[g_rawDataPtr->raw_ctr++] = result;
	}
	else {
		/* Check if no reflection is detected */
		if ((result & 0xF8) == 0x08) {
			/* set the raw date to endless (maximum value) */
			g_rawDataPtr->raw[g_rawDataPtr->raw_ctr++] = 0x7FFFFFFF;
		}
		else {
			/** @Todo: Error handling */
		}
	}

	/* Make the TDC ready for a measurement */
	bsp_GP22SendOpcode(GP22_OP_Init);
}


/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
