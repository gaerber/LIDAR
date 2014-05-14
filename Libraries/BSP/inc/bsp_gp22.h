/**
 * \file        bsp_gp22.h
 * \brief       Supports all functions to manage the TDC GP22.
 * \date        2014-05-12
 * \version     0.1
 * \author		Kevin Gerber
 *
 * \addtogroup  bsp
 * @{
 *
 * \addtogroup  bsp_gp22
 * \brief		Supports all functions to manage the TDC GP22. The GP22
 * 				is an time to digital converter with a resolution of 90ps.
 * 				It is used in measurement mode 1, which has an operating
 * 				range from 0 to 2.4us.
 * @{
 */

#ifndef BSP_GP22_H_
#define BSP_GP22_H_

#include "bsp.h"

/**
 * \typedef	bsp_gp22callback_t
 * \brief	Interrupt callback function called after a measurement.
 */
typedef void (*bsp_gp22callback_t)(void);

/*
 * ----------------------------------------------------------------------------
 * TDC configurations
 * ----------------------------------------------------------------------------
 */
#define BSP_GP22_REG0		0x00, 0x24, 0x20, 0x00		/*!< TDC-GP22 register 0 configuration */
#define BSP_GP22_REG1		0x19, 0x49, 0x00, 0x00		/*!< TDC-GP22 register 1 configuration */
#define BSP_GP22_REG2		0xA0, 0x00, 0x00, 0x00		/*!< TDC-GP22 register 2 configuration */
#define BSP_GP22_REG3		0x00, 0x00, 0x00, 0x00		/*!< TDC-GP22 register 3 configuration */
#define BSP_GP22_REG4		0x20, 0x00, 0x00, 0x00		/*!< TDC-GP22 register 4 configuration */
#define BSP_GP22_REG5		0x10, 0x00, 0x00, 0x00		/*!< TDC-GP22 register 5 configuration */
#define BSP_GP22_REG6		0x00, 0x00, 0x00, 0x00		/*!< TDC-GP22 register 6 configuration */


/**
 * \brief	A list of all operation codes from the GP22.
 */
typedef enum {
	WR_REG_0 = 0x80,		/*!< Write into register 0 */
	WR_REG_1 = 0x81,		/*!< Write into register 1 */
	WR_REG_2 = 0x82,		/*!< Write into register 2 */
	WR_REG_3 = 0x83,		/*!< Write into register 3 */
	WR_REG_4 = 0x84,		/*!< Write into register 4 */
	WR_REG_5 = 0x85,		/*!< Write into register 5 */
	WR_REG_6 = 0x86,		/*!< Write into register 6 */

	RD_RES_0 = 0xB0,		/*!< Read from result register 0 */
	RD_RES_1 = 0xB1,		/*!< Read from result register 1 */
	RD_RES_2 = 0xB2,		/*!< Read from result register 2 */
	RD_RES_3 = 0xB3,		/*!< Read from result register 3 */
	RD_STAT = 0xB4,			/*!< Read the status register */
	RD_REG_1 = 0xB5,		/*!< Read the highest 8 bits of write register 1, to be used for testing the communication */

	RD_IDBIT = 0xB7,		/*!< Read ID bits */
	RD_PW1ST = 0xB8,		/*!< Read the ratio of the width of the first half wave (at a given offset) compared to the half period of the received signal. */

	OP_Init = 0x70,			/*!< Initialize the GP22: Time measurement could be started */
	OP_Power_On_Reset = 0x50,	/*!< Reset the GP22: Must be done before the configurations */
	OP_Start_TOF = 0x01,	/*!< triggers a sequence for a single time-of-flight measurement. The sequence is made by the fire generator. */
	OP_Start_Temp = 0x02,	/*!< Triggers a single temperature measurement sequence */
	OP_Start_Cal_Resonator = 0x03,	/*!< Triggers a calibration measurement of the high speed oscillator. */
	OP_Start_Cal_TDC = 0x04,	/*!< This command starts a measurement of 2 periods of the reference clock (high speed oscilator). It is used to update the calibration raw data. */
	OP_Start_TOF_Restart = 0x05,	/*!< This opcode runs the Start_TOF sequence twice, in up and down direction as it is typical in ultrasonic flow meters. */
	OP_Start_Temp_Restart = 0x06	/*!< This opcode runs the Start_Temp sequence twice. */

} gp22_opcode_t;



/*
 * ----------------------------------------------------------------------------
 * Hardware configurations
 * ----------------------------------------------------------------------------
 */

/**
 * Interrupt pin from the GP22. Signals when measurement is finished.
 */
static const bsp_gpioconf_t BSP_GP22_INT = {
		RCC_AHB1Periph_GPIOG, GPIOG, GPIO_Pin_8, GPIO_Mode_IN, GPIO_PuPd_UP
};

/* Interrupt settings */
#define BSP_GP22_IRQ_CHANEL		EXTI9_5_IRQn		/*!< NVIC GPIO interrupt */
#define BSP_GP22_IRQ_PRIORITY	8					/*!< NVIC GPIO interrupt priority */
#define BSP_GP22_IRQ_Handler	EXTI9_5_IRQHandler	/*!< NVIC GPIO handler */


/*
 * ----------------------------------------------------------------------------
 * Prototypes
 * ----------------------------------------------------------------------------
 */
extern void bsp_GP22Init(void);
extern void bsp_GP22IntCallback(bsp_gp22callback_t int_callback);
extern uint8_t bsp_GP22Opcode(gp22_opcode_t op);

#endif /* BSP_GP22_H_ */

/**
 * @}
 */

/**
 * @}
 */
