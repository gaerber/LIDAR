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

/* Application */
#include "data_acquisition.h"

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
	bsp_QuadencSetCapture(DA_AZIMUTH_MAX + 2 * DA_AZIMUTH_RES);
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
		bsp_QuadencSetCapture(1);

		/* Disable the automatic calibration calculation on the TDC */
		reg = BSP_GP22_REG0 & (~(1<<13));
		bsp_GP22RegWrite(GP22_WR_REG_0, reg);

		/* Disable the fast init feature */
		reg = BSP_GP22_REG1 & (~(1<<23));
		bsp_GP22RegWrite(GP22_WR_REG_1, reg);

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
	uint32_t result;

	/* Read the state register */
//	bsp_GP22RegRead(GP22_RD_STAT, &result, 2);
	result=0;

	/* Check the state */
	if (result == 0x00) {
		/* Read the calibration value */
		bsp_GP22RegRead(GP22_RD_RES_0, &result, 4);
	}
	else {
		/** @Todo: Error handling */
	}

	/* Reset the configuration */
	bsp_GP22RegWrite(GP22_WR_REG_0, BSP_GP22_REG0);
	bsp_GP22RegWrite(GP22_WR_REG_1, BSP_GP22_REG1);
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
	/* Check if it is enabled */
	if (g_settings.enable) {
		/* Configure the next azimuth interrupt: First measurement point */
		bsp_QuadencPosCallback(azimuthMeasurementHandler);
		bsp_QuadencSetCapture(g_settings.azimuth_left);

		/* Set the TDC callback function */
		bsp_GP22IntCallback(tdcPropagationDelayCalibrationHandler);

		/* Make the TDC ready */
		bsp_GP22SendOpcode(GP22_OP_Init);

		/* Starts a measurement */
		bsp_LaserPulse(g_settings.laser_pulses);
	}
	else {
		/* Data acquisition disable */
		bsp_QuadencPosCallback(NULL);
	}
}

/**
 * \brief	TDC interrupt handler, called after the propagation delay measurement.
 */
void tdcPropagationDelayCalibrationHandler(void) {
	uint32_t result;

	/* Read the state register */
//	bsp_GP22RegRead(GP22_RD_STAT, &result, 2);
	result=0x48;

	/* Check the state */
	if (result == 0x48) {
		/* Read the calibration value */
		bsp_GP22RegRead(GP22_RD_RES_0, &result, 4);
	}
	else {
		/** @Todo: Error handling */
	}
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
			bsp_QuadencSetCapture(DA_AZIMUTH_MAX + 2 * DA_AZIMUTH_RES);
		}

		/* Set the TDC callback function */
		bsp_GP22IntCallback(tdcMeasurementHandler);

		/* Starts the measurement */
		bsp_LaserPulse(g_settings.laser_pulses);
	}
	else {
		/* Data acquisition disable */
		bsp_QuadencPosCallback(NULL);
	}
}

/**
 * \brief	TDC interrupt handler, called after the propagation delay measurement.
 */
void tdcMeasurementHandler(void) {
	uint32_t result;

	/* Read the state register */
//	bsp_GP22RegRead(GP22_RD_STAT, &result, 2);
	result=0x48;

	/* Check the state */
	if (result == 0x48) {
		/* Read the calibration value */
		bsp_GP22RegRead(GP22_RD_RES_0, &result, 4);
	}
	else {
		/** @Todo: Error handling */
	}
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
