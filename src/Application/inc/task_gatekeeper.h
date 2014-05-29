/**
 * \file		task_gatekeeper.h
 * \brief		Sends all messages over the serial interface to the user.
 * \date		2014-05-29
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	app
 * @{
 *
 * \addtogroup	task_gatekeeper
 * \brief		The gatekeeper tasks manages all messages, which have to be
 * 				transmitted over the serial interface.
 * @{
 */

#ifndef TASK_GATEKEEPER_H_
#define TASK_GATEKEEPER_H_

/*
 * ----------------------------------------------------------------------------
 * Task settings
 * ----------------------------------------------------------------------------
 */
#define TASK_GATEKEEPER_NAME		"Gatekeeper"				/*!< Task name. */
#define TASK_GATEKEEPER_PRIORITY	2							/*!< Task Priority. */
#define TASK_GATEKEEPER_STACKSIZE	configMINIMAL_STACK_SIZE	/*!< Task Stack size. */


/*
 * ----------------------------------------------------------------------------
 * Task synchronization settings
 * ----------------------------------------------------------------------------
 */
#define Q_MESSAGE_LENGTH			10		/*!< Queue length of the messages. */
#define MESSAGE_STRING_LENGTH		24		/*!< Maximal length of each message. */


/*
 * ----------------------------------------------------------------------------
 * Application: Message types
 * ----------------------------------------------------------------------------
 */
#define MSG_TYPE_ECHO			'>'		/*!< User input echo message. */
#define MSG_TYPE_RSP			'='		/*!< Response message of a user command. */
#define MSG_TYPE_CONF			'@'		/*!< A configuration value. */
#define MSG_TYPE_STATE			'#'		/*!< System state message. */
#define MSG_TYPE_DATA			'$'		/*!< Data point of the room map. */

#define IS_MSG_TYPE(mt) (((mt) == MSG_TYPE_ECHO) || ((mt) == MSG_TYPE_RSP)|| \
				((mt) == MSG_TYPE_CONF) || ((mt) == MSG_TYPE_STATE) \
				((mt) == MSG_TYPE_DATA))

#define MSG_FRAME_END			"\r\n"	/*!< End of a message frame */

/*
 * ----------------------------------------------------------------------------
 * Type declarations
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Data type of a message. One message will be placed in one frame.
 */
typedef struct {
	char type;							/*!< Message type. */
	char msg[MESSAGE_STRING_LENGTH];	/*!< Message, without the frame. */
} message_t;


/*
 * ----------------------------------------------------------------------------
 * Task synchronization
 * ----------------------------------------------------------------------------
 */
extern TaskHandle_t taskGatekeeperHandle;
extern QueueHandle_t queueMessage;
extern SemaphoreHandle_t mutexTxCircBuf;


/*
 * ----------------------------------------------------------------------------
 * Prototypes
 * ----------------------------------------------------------------------------
 */
extern void taskGatekeeperInit(void);


#endif /* TASK_GATEKEEPER_H_ */

/**
 * @}
 */

/**
 * @}
 */
