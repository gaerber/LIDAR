/**
 * \file		task_scanner.h
 * \brief		Controls the rotation of the mirror.
 * \date		2014-05-28
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	app
 * @{
 *
 * \addtogroup	task_scanner
 * \brief		The scanner task makes the rotation of the mirror. The movement
 * 				is controlled by a softwarePI controller.
 * @{
 */

#ifndef TASK_SCANNER_H_
#define TASK_SCANNER_H_


/*
 * ----------------------------------------------------------------------------
 * Task settings
 * ----------------------------------------------------------------------------
 */
#define TASK_SCANNER_NAME			"Scanner"					/*!< Task name. */
#define TASK_SCANNER_PRIORITY		7							/*!< Task priority. */
#define TASK_SCANNER_STACKSIZE		configMINIMAL_STACK_SIZE	/*!< Task stack size. */


/*
 * ----------------------------------------------------------------------------
 * Task synchronization settings
 * ----------------------------------------------------------------------------
 */
#define Q_SPEED_LENGTH				1		/*!< Queue length of the messages. */


/*
 * ----------------------------------------------------------------------------
 * Application settings
 * ----------------------------------------------------------------------------
 */
#define ENGINE_CONTROLER_KP			180		/*!< Proportional gain. */
#define ENGINE_CONTROLER_KI			55		/*!< Integral gain. */
#define ENGINE_CONTROLER_TA			1		/*!< Time interval. */
#define ENGINE_SETTING_TIME			800		/*!< Engine speed controller setting time [ms] */
#define ENGINE_RISE_TIME			180		/*!< Engine speed controller rise time [ms] */
#define ENGINE_MAX_POWER			4199	/*!< Maximum power. Must be smaller than BSP_ENGINE_PWM_PERIOD! */


/*
 * ----------------------------------------------------------------------------
 * Type declarations
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Data type of the rotation speed.
 */
typedef int32_t speed_t;


/*
 * ----------------------------------------------------------------------------
 * Task synchronization
 * ----------------------------------------------------------------------------
 */
extern TaskHandle_t taskScannerHandle;
extern QueueHandle_t queueSpeed;


/*
 * ----------------------------------------------------------------------------
 * Prototypes
 * ----------------------------------------------------------------------------
 */
extern void taskScannerInit(void);


#endif /* TASK_SCANNER_H_ */

/**
 * @}
 */

/**
 * @}
 */
