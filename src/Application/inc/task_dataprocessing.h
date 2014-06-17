/**
 * \file		task_dataprocessing.h
 * \brief		Data processing task.
 * \date		2014-06-02
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	app
 * @{
 *
 * \addtogroup	task_dataprocessing
 * \brief		Processes the raw data form the TDC-GP22. This task estimate
 * 				the propagation time of the laser and calculates the distance.
 * 				Further, the azimuth and the distance is encoded.
 * @{
 */

#ifndef TASK_DATAPROCESSING_H_
#define TASK_DATAPROCESSING_H_

/*
 * ----------------------------------------------------------------------------
 * Task settings
 * ----------------------------------------------------------------------------
 */
#define TASK_DATAPROC_NAME			"Data Processing"			/*!< Task name. */
#define TASK_DATAPROC_PRIORITY		2							/*!< Task Priority. */
#define TASK_DATAPROC_STACKSIZE		configMINIMAL_STACK_SIZE	/*!< Task Stack size. */


/*
 * ----------------------------------------------------------------------------
 * Task synchronization settings
 * ----------------------------------------------------------------------------
 */
#define Q_RAWDATA_LENGTH			30			/*!< Memory pool and queue length of the raw data. */
#define MAX_RAWDATA_LENGTH			25			/*!< Maximum measurement points each point of the room map. */


/*
 * ----------------------------------------------------------------------------
 * Application settings
 * ----------------------------------------------------------------------------
 */
#define VERILOG_OF_LIGHT			299792458	/*!< Verilog of the light [m/s]. Source: Wikipedia */


/*
 * ----------------------------------------------------------------------------
 * Type declarations
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Raw data structure of a point of the room map.
 */
typedef struct {
	uint32_t increments;		/*!< Azimuth in increments. */
	uint32_t cal_resonator;		/*!< Raw calibration value of the resonator. */
	uint32_t expected_points;	/*!< Number of expected raw data points. */
	uint32_t raw_ctr;			/*!< Raw data counter. */
	uint32_t raw[MAX_RAWDATA_LENGTH];	/*!< Raw data. */
} rawdata_t;


/*
 * ----------------------------------------------------------------------------
 * Task synchronization
 * ----------------------------------------------------------------------------
 */
extern TaskHandle_t taskDataProcessingHandle;
extern QueueHandle_t queueRawDataPtr;
extern MemPoolManager memRawData;


/*
 * ----------------------------------------------------------------------------
 * Prototypes
 * ----------------------------------------------------------------------------
 */
extern void taskDataProcessingInit(void);


#endif /* TASK_DATAPROCESSING_H_ */

/**
 * @}
 */

/**
 * @}
 */
