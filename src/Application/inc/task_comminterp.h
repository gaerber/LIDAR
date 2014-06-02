/**
 * \file		task_comminterp.h
 * \brief		Interpret the received commands from the user.
 * \date		2014-05-28
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	app
 * @{
 *
 * \addtogroup	task_comminterp
 * \brief		The comment interpreter task receives the commands form the user
 * 				and check them. The resolved commands were send to the controller
 * 				task.
 * @{
 */

#ifndef TASK_COMMINTERP_H_
#define TASK_COMMINTERP_H_


/*
 * ----------------------------------------------------------------------------
 * Task settings
 * ----------------------------------------------------------------------------
 */
#define TASK_COMMINTERP_NAME		"Comm. Interp."				/*!< Task name. */
#define TASK_COMMINTERP_PRIORITY	2							/*!< Task Priority. */
#define TASK_COMMINTERP_STACKSIZE	configMINIMAL_STACK_SIZE	/*!< Task Stack size. */


/*
 * ----------------------------------------------------------------------------
 * Task synchronization settings
 * ----------------------------------------------------------------------------
 */
#define Q_READCOMMAND_LENGTH		1		/*!< Queue length of the messages. */


/*
 * ----------------------------------------------------------------------------
 * Application settings
 * ----------------------------------------------------------------------------
 */
#define COMMAND_BUFFER_LENGTH		64		/*!< Buffer length of the received commands. */


/*
 * ----------------------------------------------------------------------------
 * Type declarations
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Data type of the queueReadCommand. Enable or disable the echo of
 * 			the command interpreter.
 */
typedef uint8_t readcommand_t;


/*
 * ----------------------------------------------------------------------------
 * Task synchronization
 * ----------------------------------------------------------------------------
 */
extern TaskHandle_t taskCommInterpHandle;
extern QueueHandle_t queueReadCommand;


/*
 * ----------------------------------------------------------------------------
 * Prototypes
 * ----------------------------------------------------------------------------
 */
extern void taskCommInterpInit(void);


#endif /* TASK_COMMINTERP_H_ */

/**
 * @}
 */

/**
 * @}
 */
