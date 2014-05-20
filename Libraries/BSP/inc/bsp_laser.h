/**
 * \file		bsp_laser.h
 * \brief		Supports the laser pulse generator.
 * \date		2014-05-13
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	bsp
 * @{
 *
 * \addtogroup	bsp_laser
 * \brief		This Module provides the functions to configure the laser pulse generator.
 * @{
 */

#ifndef BSP_LASER_H_
#define BSP_LASER_H_

#include "bsp.h"


/*
 * ----------------------------------------------------------------------------
 * Pulse settings
 * ----------------------------------------------------------------------------
 */

/** PWM frequency of the laser pulse generator. */
#define BSP_LASER_FREQ				84000000
/** Period register of the PWM. The frequency of the laser pulse replay is BSP_LASER_FREQ[Hz] / (BSP_LASER_PERIOD-1)  */
#define BSP_LASER_PERIOD			1681
/** Laser pulse width. The duty cycle is \f[ D = \frac{BSP_LASER_PULSE_WIDTH}{BSP_LASER_PERIOD-1} \f] */
#define BSP_LASER_PULSE_WIDTH		3


/*
 * ----------------------------------------------------------------------------
 * Hardware configurations
 * ----------------------------------------------------------------------------
 */

/** RCC AHB peripheral of the timer port. */
#define BSP_LASER_TIMER_PORT_PERIPH	RCC_APB1Periph_TIM5
/** Port base address of the timer port */
#define BSP_LASER_TIMER_PORT_BASE		TIM5
/** Used PWM type */
#define BSP_LASER_TIMER_PORT_CHANEL		CHANEL1

/** Hardware label of the PWM output pin, which is connected to the engine. */
static const bsp_gpioconf_t BSP_LASER_PORT = {
		RCC_AHB1Periph_GPIOA, GPIOA, GPIO_Pin_0, GPIO_Mode_AF, GPIO_PuPd_NOPULL, GPIO_AF_TIM5
};

/* Interrupt settings */
#define BSP_LASER_IRQ_CHANEL		TIM5_IRQn			/*!< NVIC timer interrupt */
#define BSP_LASER_IRQ_SOURCE		TIM_IT_CC1			/*!< NVIC timer interrupt source */
#define BSP_LASER_IRQ_PRIORITY		2					/*!< NVIC timer interrupt priority */
#define BSP_LASER_IRQ_Handler		TIM5_IRQHandler		/*!< NVIC timer handler */


/*
 * ----------------------------------------------------------------------------
 * Prototypes
 * ----------------------------------------------------------------------------
 */
extern void bsp_LaserInit(void);
extern void bsp_LaserPulse(uint32_t nr_of_pulses);


#endif /* BSP_LASER_H_ */

/**
 * @}
 */

/**
 * @}
 */
