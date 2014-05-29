/**
 * \file		task_controller.h
 * \brief		Contains the controller task.
 * \date		2014-05-28
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	app
 * @{
 *
 * \addtogroup	task_controller
 * \brief		The controller processes the resolved commands and any system
 * 				error messages.
 * @{
 */

#ifndef TASK_CONTROLLER_H_
#define TASK_CONTROLLER_H_


/*
 * ----------------------------------------------------------------------------
 * Task settings
 * ----------------------------------------------------------------------------
 */
#define TASK_CONTROLLER_NAME		"Controller"				/*!< Task name. */
#define TASK_CONTROLLER_PRIORITY	2							/*!< Task Priority. */
#define TASK_CONTROLLER_STACKSIZE	configMINIMAL_STACK_SIZE	/*!< Task Stack size. */


/*
 * ----------------------------------------------------------------------------
 * Task synchronization settings
 * ----------------------------------------------------------------------------
 */
#define Q_COMMAND_LENGTH			5		/*!< Queue length of the received commands. */


/*
 * ----------------------------------------------------------------------------
 * Type declarations
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Data type of the queueCommand. It is a structure with the command
 * 			and the new parameters.
 */
typedef struct {
	enum {
		Init,
		Start
	} command;

	union {
		uint8_t echo; /*!< 1 := ON; 0 := OFF */
		uint8_t respmsg; /*!< 1:= message + message code; 0:= only the message */
		uint8_t speed; /*!< speed in % from 0 to 100 */
		uint8_t brightness;	/*!< brightness of the flashlight from 0 to 100 */
		uint8_t error; /*!< error ID's, \see TaskController.h */
	} param;
} command_t;


/*
 * ----------------------------------------------------------------------------
 * Task synchronization
 * ----------------------------------------------------------------------------
 */
extern TaskHandle_t taskControllerHandle;
extern QueueHandle_t queueCommand;


/*
 * ----------------------------------------------------------------------------
 * Prototypes
 * ----------------------------------------------------------------------------
 */
extern void taskControllerInit(void);


#endif /* TASK_CONTROLLER_H_ */

/**
 * @}
 */

/**
 * @}
 */
