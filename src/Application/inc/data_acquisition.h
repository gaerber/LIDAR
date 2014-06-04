/**
 * \file		data_acquisition.h
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
 * \brief		The data acquisition measures the azimuth and the
 * 				distance. The azimuth is giver by a quadrature encoder
 * 				and the distance has to be calculated with the captured
 * 				time of flight.
 * @{
 */

#ifndef DATA_ACQUISITION_H_
#define DATA_ACQUISITION_H_


/*
 * ----------------------------------------------------------------------------
 * Configurations
 * ----------------------------------------------------------------------------
 */
#define DA_AZIMUTH_MIN		-1188	/*!< Minimum azimuth [tenth degree]. */
#define DA_AZIMUTH_MAX		1188	/*!< Maximum azimuth [tenth degree]. */
#define DA_AZIMUTH_RES		18		/*!< Default azimuth steps [tenth degree]. */
#define DA_AZIMUTH_CAL_DIST	1		/*!< Azimuth at which the distance is calibrated. */
#define DA_AZIMUTH_CAL_RES	(DA_AZIMUTH_MAX + 2 * DA_AZIMUTH_RES)	/*!< Azimuth at which the high speed clock is calibrated. */

#define DA_DEF_SCANRATE		2		/*!< Default scan rate in scans per seconds. */
#define DA_LASERPULSE		50		/*!< Number of laser pulse with 1 scan per second. */

/*
 * ----------------------------------------------------------------------------
 * Type declarations
 * ----------------------------------------------------------------------------
 */



/*
 * ----------------------------------------------------------------------------
 * Prototypes
 * ----------------------------------------------------------------------------
 */
extern void DataAcquisitionInit(void);
extern void DataAcquisitionStart(uint32_t atzimuth_left, uint32_t azimuth_right,
		uint32_t azimuth_res, uint32_t laser_pulses);
extern void DataAcquisitionStop(void);

#endif /* DATA_ACQUISITION_H_ */

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
