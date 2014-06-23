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
#define TASK_DATAACQUISITION_PRIORITY	6							/*!< Task Priority. */
#define TASK_DATAACQUISITION_STACKSIZE	configMINIMAL_STACK_SIZE	/*!< Task Stack size. */


/*
 * ----------------------------------------------------------------------------
 * Task synchronization settings
 * ----------------------------------------------------------------------------
 */
#define Q_DATAACQUISITION_LENGTH		2		/*!< Queue length of the data acquisition settings. */


/*
 * ----------------------------------------------------------------------------
 * Configurations
 * ----------------------------------------------------------------------------
 */
#define DA_LASERPULSE		30		/*!< Number of laser pulse with 1 scan per second. */


/*
 * ----------------------------------------------------------------------------
 * Type declarations
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Data acquisition configurations.
 */
typedef struct {
	enum {
		DATA_ACQUISITION_ENABLE,	/*!< Starts the data acquisition. */
		DATA_ACQUISITION_DISABLE	/*!< Stops the data acquisition. */
	} state;						/*!< New state of the data acquisition. */
	union {
		struct {
			int16_t bndry_left;		/*!< Configured scan area boundary left. [tenth degree] */
			int16_t bndry_right;	/*!< Configured scan area boundary right. [tenth degree] */
			int16_t step;			/*!< Configures step size between two measurement points. [tenth degree] */
			uint8_t rate;			/*!< Configured update rate of the hole room map. [turns per second] */
		} scan;						/*!< Scan settings. */
		uint16_t engine_sleep;		/*!< Configured time delay before the engine is suspended in CMD mode. [ms] */
	} param;						/*!< Parameter of the new data acquisition state. */
} dataacquisition_t;


/*
 * ----------------------------------------------------------------------------
 * Task synchronization
 * ----------------------------------------------------------------------------
 */
extern TaskHandle_t taskDataAcquisitionHandle;
extern QueueHandle_t queueDataAcquisition;


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
