/**
 * \file		bsp_gp22.h
 * \brief		Supports all functions to manage the TDC GP22.
 * \date		2014-05-12
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	bsp
 * @{
 *
 * \addtogroup	bsp_gp22
 * \brief		Supports all functions to manage the TDC GP22. The GP22
 * 				is an time to digital converter with a resolution of 90ps.
 * 				It is used in measurement mode 1, which has an operating
 * 				range from 0 to 2.4us.
 * @{
 */

#ifndef BSP_GP22_H_
#define BSP_GP22_H_

#include "bsp.h"

/*
 * ----------------------------------------------------------------------------
 * Type declarations
 * ----------------------------------------------------------------------------
 */

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
#define BSP_GP22_REG0		0x00042655		/*!< TDC-GP22 register 0 configuration */
#define BSP_GP22_REG1		0x19C900AA		/*!< TDC-GP22 register 1 configuration */
#define BSP_GP22_REG2		0x20000000		/*!< TDC-GP22 register 2 configuration */
#define BSP_GP22_REG3		0x000000FF		/*!< TDC-GP22 register 3 configuration */ /* EN_ERR_VAL FUNKTIONIERT NICHT WIE ERWARTET */
#define BSP_GP22_REG4		0x20000000		/*!< TDC-GP22 register 4 configuration */
#define BSP_GP22_REG5		0x10000000		/*!< TDC-GP22 register 5 configuration */
#define BSP_GP22_REG6		0x00000000		/*!< TDC-GP22 register 6 configuration */

#define BSP_GP22_RESONATOR	32768.0			/*!< Frequency of the calibration resonator [Hz]. */
#define BSP_GP22_RESONATOR_CYCLE	2.0		/*!< Number of cycles while resonator calibration. */
#define BSP_GP22_HS_CRYSTAL	4000000.0		/*!< Frequency of the high speed crystal [Hz]. */

/*
 * ----------------------------------------------------------------------------
 * Hardware configurations
 * ----------------------------------------------------------------------------
 */

/**
 * Interrupt pin from the GP22. Signals when measurement is finished.
 */
static const bsp_gpioconf_t BSP_GP22_INT = {
		RCC_AHB1Periph_GPIOB, GPIOB, GPIO_Pin_1, GPIO_Mode_IN, GPIO_PuPd_UP
};

/* Interrupt settings */
#define BSP_GP22_IRQ_CHANEL		EXTI1_IRQn			/*!< NVIC GPIO interrupt */
#define BSP_GP22_IRQ_PRIORITY	8					/*!< NVIC GPIO interrupt priority */
#define BSP_GP22_IRQ_Handler	EXTI1_IRQHandler	/*!< NVIC GPIO handler */


/*
 * ----------------------------------------------------------------------------
 * TDC communication protocol
 * ----------------------------------------------------------------------------
 */

/* TDC-GP22 write registers to configure the system. */
#define GP22_WR_REG_0		((uint8_t)0x80)		/*!< Write into register 0 */
#define GP22_WR_REG_1		((uint8_t)0x81)		/*!< Write into register 1 */
#define GP22_WR_REG_2		((uint8_t)0x82)		/*!< Write into register 2 */
#define GP22_WR_REG_3		((uint8_t)0x83)		/*!< Write into register 3 */
#define GP22_WR_REG_4		((uint8_t)0x84)		/*!< Write into register 4 */
#define GP22_WR_REG_5		((uint8_t)0x85)		/*!< Write into register 5 */
#define GP22_WR_REG_6		((uint8_t)0x86)		/*!< Write into register 6 */

#define GP22_IS_WR(WR) (((WR) == WR_REG_0) || ((WR) == WR_REG_1)|| \
				((WR) == WR_REG_2) || ((WR) == WR_REG_3) \
				((WR) == WR_REG_4) || ((WR) == WR_REG_5) \
				((WR) == WR_REG_6))


/* TDC-GP22 read registers. */
#define GP22_RD_RES_0		((uint8_t)0xB0)		/*!< Read from result register 0 */
#define GP22_RD_RES_1		((uint8_t)0xB1)		/*!< Read from result register 1 */
#define GP22_RD_RES_2		((uint8_t)0xB2)		/*!< Read from result register 2 */
#define GP22_RD_RES_3		((uint8_t)0xB3)		/*!< Read from result register 3 */
#define GP22_RD_STAT		((uint8_t)0xB4)		/*!< Read the status register */
#define GP22_RD_REG_1		((uint8_t)0xB5)		/*!< Read the highest 8 bits of write register 1, to be used for testing the communication */
#define GP22_RD_IDBIT		((uint8_t)0xB7)		/*!< Read ID bits */
#define GP22_RD_PW1ST		((uint8_t)0xB8)		/*!< Read the ratio of the width of the first half wave (at a given offset) compared to the half period of the received signal. */

#define GP22_IS_RD(RD) (((RD) == RD_RES_0) || ((RD) == RD_RES_1)|| \
				((RD) == RD_RES_2) || ((RD) == RD_RES_3) \
				((RD) == RD_STAT) || ((RD) == RD_REG_1) \
				((RD) == RD_IDBIT) || ((RD) == RD_PW1ST))


/* TDC-GP22 operation codes. */
#define GP22_OP_Init 				((uint8_t)0x70)	/*!< Initialize the GP22: Time measurement could be started */
#define GP22_OP_Power_On_Reset		((uint8_t)0x50)	/*!< Reset the GP22: Must be done before the configurations */
#define GP22_OP_Start_TOF			((uint8_t)0x01)	/*!< triggers a sequence for a single time-of-flight measurement. The sequence is made by the fire generator. */
#define GP22_OP_Start_Temp			((uint8_t)0x02)	/*!< Triggers a single temperature measurement sequence */
#define GP22_OP_Start_Cal_Resonator	((uint8_t)0x03)	/*!< Triggers a calibration measurement of the high speed oscillator. */
#define GP22_OP_Start_Cal_TDC		((uint8_t)0x04)	/*!< This command starts a measurement of 2 periods of the reference clock (high speed oscilator). It is used to update the calibration raw data. */
#define GP22_OP_Start_TOF_Restart	((uint8_t)0x05)	/*!< This opcode runs the Start_TOF sequence twice, in up and down direction as it is typical in ultrasonic flow meters. */
#define GP22_OP_Start_Temp_Restart	((uint8_t)0x06)	/*!< This opcode runs the Start_Temp sequence twice. */

#define GP22_IS_OP(OP) (((OP) == OP_Init) || ((OP) == OP_Power_On_Reset)|| \
				((OP) == OP_Start_TOF) || ((OP) == OP_Start_Temp) \
				((OP) == OP_Start_Cal_Resonator) || ((OP) == OP_Start_Cal_TDC) \
				((OP) == OP_Start_TOF_Restart) || ((OP) == OP_Start_Temp_Restart))


/*
 * ----------------------------------------------------------------------------
 * Prototypes
 * ----------------------------------------------------------------------------
 */
extern void bsp_GP22Init(void);
extern void bsp_GP22IntCallback(bsp_gp22callback_t int_callback);
extern uint8_t bsp_GP22SendOpcode(uint8_t op);
extern uint8_t bsp_GP22RegWrite(uint8_t reg, uint32_t new_reg_val);
extern uint8_t bsp_GP22RegRead(uint8_t reg, uint32_t *value, uint8_t len);

#endif /* BSP_GP22_H_ */

/**
 * @}
 */

/**
 * @}
 */
