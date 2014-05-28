/**
 * \file		task_dataacquisition.h
 * \brief		The data acquisition contains the azimuth and the distance.
 * \date		2014-05-28
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	app
 * @{
 *
 * \addtogroup	task_dataacquisition
 * \brief		The data acquisition task measures the azimuth and the
 * 				distance. The azimuth is giver by a quadrature encoder
 * 				and the distance has to be calculated with the captured
 * 				time of flight.
 * @{
 */

#ifndef TASK_DATAACQUISITION_H_
#define TASK_DATAACQUISITION_H_


/*
 * ----------------------------------------------------------------------------
 * Task settings
 * ----------------------------------------------------------------------------
 */
#define TASK_DATAACQUISITION_NAME		"Acquisition"				/*!< Task name. */
#define TASK_DATAACQUISITION_PRIORITY	2							/*!< Task Priority. */
#define TASK_DATAACQUISITION_STACKSIZE	configMINIMAL_STACK_SIZE	/*!< Task Stack size. */


/*
 * ----------------------------------------------------------------------------
 * Task synchronization settings
 * ----------------------------------------------------------------------------
 */



/*
 * ----------------------------------------------------------------------------
 * Type declarations
 * ----------------------------------------------------------------------------
 */



/*
 * ----------------------------------------------------------------------------
 * Task synchronization
 * ----------------------------------------------------------------------------
 */
extern TaskHandle_t taskDataAcquisitionHandle;


/*
 * ----------------------------------------------------------------------------
 * Prototypes
 * ----------------------------------------------------------------------------
 */
extern void taskDataAcquisitionInit(void);


#endif /* TASK_DATAACQUISITION_H_ */

/**
 * @}
 */

/**
 * @}
 */
