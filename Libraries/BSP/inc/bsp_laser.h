/**
 * \file		bsp_laser.h
 * \brief		Supports the laser pulse generator.
 * \date		2014-05-26
 * \version		0.2
 * \author		Kevin Gerber
 *
 * \addtogroup	bsp
 * @{
 *
 * \addtogroup	bsp_laser
 * \brief		This Module provides the functions to configure the laser pulse
 * 				generator. The output is switched as PWM and the center aligned
 * 				counter mode is set. In this case the pulse is in the middle of
 * 				a period and a sequence stop in high state in not possible.
 * \warning		Only TIM1 and TIM8 are allowed in this module, due to the
 * 				necessary repetition counter register.
 * @{
 */

#ifndef BSP_LASER_H_
#define BSP_LASER_H_

#include "bsp.h"


/*
 * ----------------------------------------------------------------------------
 * Type declarations
 * ----------------------------------------------------------------------------
 */

/**
 * \typedef	bsp_lasercallback_t
 * \brief	Interrupt callback function called after a laser pulse sequence.
 */
typedef void (*bsp_lasercallback_t)(void);


/*
 * ----------------------------------------------------------------------------
 * Pulse settings
 * ----------------------------------------------------------------------------
 */

/** PWM frequency of the laser pulse generator. */
#define BSP_LASER_FREQ				84000000
/** Period register of the PWM. The frequency of the laser pulse replay is f = 1/2 * BSP_LASER_FREQ[Hz] / (BSP_LASER_PERIOD-1)  */
#define BSP_LASER_PERIOD			(5*2*841)
/** Laser pulse width. The duty cycle is D = BSP_LASER_PULSE_WIDTH / (BSP_LASER_PERIOD-1) */
#define BSP_LASER_PULSE_WIDTH		10


/*
 * ----------------------------------------------------------------------------
 * Hardware configurations
 * ----------------------------------------------------------------------------
 */

/** Hardware label of overcurrent measurement input (low active). */
static const bsp_gpioconf_t BSP_LASER_NER_PORT = {
		RCC_AHB1Periph_GPIOC, GPIOC, GPIO_Pin_2, GPIO_Mode_IN, GPIO_PuPd_UP
};

/** RCC AHB peripheral of the timer port. */
#define BSP_LASER_TIMER_PORT_PERIPH		RCC_APB2Periph_TIM8
/** Port base address of the timer port */
#define BSP_LASER_TIMER_PORT_BASE		TIM8
/** Used PWM channel */
#define BSP_LASER_TIMER_PORT_CHANEL		CHANNEL1

/** Hardware label of the PWM output pin, which generates the laser pulses. */
static const bsp_gpioconf_t BSP_LASER_PORT = {
		RCC_AHB1Periph_GPIOC, GPIOC, GPIO_Pin_6, GPIO_Mode_AF, GPIO_PuPd_DOWN, GPIO_AF_TIM8
};

/* Laser interrupt settings */
#define BSP_LASER_IRQ_CHANEL		TIM8_UP_TIM13_IRQn	/*!< NVIC timer interrupt */
#define BSP_LASER_IRQ_SOURCE		TIM_IT_Update		/*!< NVIC timer interrupt source */
#define BSP_LASER_IRQ_PRIORITY		0					/*!< NVIC timer interrupt priority */
#define BSP_LASER_IRQ_Handler		TIM8_UP_TIM13_IRQHandler	/*!< NVIC timer handler */

/* User defined sequence end interrupt settings */
#define BSP_LASER_USR_IRQ_CHANEL	EXTI4_IRQn			/*!< NVIC timer interrupt */
#define BSP_LASER_USR_IRQ_SOURCE	EXTI_Line4			/*!< NVIC timer interrupt source */
#define BSP_LASER_USR_IRQ_PRIORITY	8					/*!< NVIC timer interrupt priority */
#define BSP_LASER_USR_IRQ_Handler	EXTI4_IRQHandler	/*!< NVIC timer handler */


/*
 * ----------------------------------------------------------------------------
 * Prototypes
 * ----------------------------------------------------------------------------
 */
extern void bsp_LaserInit(void);
extern void bsp_LaserSequenceCalback(bsp_lasercallback_t callback);
extern void bsp_LaserPulse(uint32_t nr_of_pulses);
extern uint8_t bsp_LaserOvercurrent(void);


#endif /* BSP_LASER_H_ */

/**
 * @}
 */

/**
 * @}
 */
