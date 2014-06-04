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
 * Application settings
 * ----------------------------------------------------------------------------
 */
#define LIDAR_VERSION				"0.1 RC"	/*!< LIDAR versions number [string] */


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
		/* Systems functions */
		Sys_Init = 0,		/*!< Initialize the system. Called after the system start. */
		Sys_Check,			/*!< Make a system check. */
		Sys_Welcome,		/*!< Sends the welcome text over the interface. */

		/* User commands */
		UC_Cmd,				/*!< Change into the command mode. */
		UC_Data,			/*!< Change into the data mode and starts the data acquisition. */
		UC_Reboot,			/*!< Reboot the system. */
		UC_SetCommEcho,		/*!< Enable/disable the command echo. */
		UC_SetCommRespmsg,	/*!< Enable/disable the response message. */
		UC_SetScanBndry,	/*!< Configure the scan area boundary. */
		UC_SetScanStep,		/*!< Configure the step size between two measurement points. */
		UC_SetScanRate,		/*!< Configure the update rate of the hole room map. */
		UC_SetEngineSleep,	/*!< Sets the time delay before the engine is suspended. */
		UC_GetAll,			/*!< Get all configured parameters. */
		UC_GetVer,			/*!< Get the version number. */
		UC_GetComm,			/*!< Get the communication configurations. */
		UC_GetScan,			/*!< Get the scan configurations. */
		UC_GetEngine,		/*!< Get the engine configurations. */
		UC_EE,				/*!< Some magic feature. */
		UC_Exit,			/*!< Exit the system */

		/* User Command Error */
		ErrUC_UnknownCommand,	/*!< A unknown command is received. */
		ErrUC_TooFewArgs,	/*!< To few arguments in the command. */
		ErrUC_FaultArgType,	/*!< One or more arguments were in the fault data type. */
		ErrUC_ArgOutOfBounds,	/*!< One or more arguments were out of the allowed bounds. */
		ErrUC_LineOverflow,	/*!< Command line overflow detected. */

		/* System malfunctions */
		Malf_EngineDriver,	/*!< Engine overcurrent or thermal shutdown. */
		Malf_LaserDriver,	/*!< Laser overcurrent was detected. */
		Malf_QuadEnc,		/*!< Quadrature encoder malfunction was detected. */
		Malf_Tdc			/*!< TDC stat register has an unexpected value. */
	} command;

	union {
		/* User command parameters */
		uint8_t echo;		/*!< Enable or disable the RS232 echo. */
		uint8_t respmsg;	/*!< Enable or disable the response message. */
		uint16_t engine_sleep;/*!< Ticks before the engine is suspended. */
		struct {
			int16_t left;	/*!< Left azimuth boundary. */
			int16_t right;	/*!< Right azimuth boundary. */
		} azimuth_bndry;	/*!< Azimuth boundary. */
		int16_t azimuth_step;	/*!< Azimuth step size. */
		uint8_t scan_rate;	/*!< Update rate of the room map. */
		/* User error code */
		uint8_t error_level;	/*!< Level of the command error */
		/* System malfunction parameters */
		uint16_t gp22_stat;	/*!< State register of the GP22. */
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
