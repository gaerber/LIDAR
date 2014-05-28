/**
 * \file		bsp_quadenc.h
 * \brief		Supports all functions to capture the azimuth.
 * \date		2014-05-20
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	bsp
 * @{
 *
 * \addtogroup	bsp_quadenc
 * \brief		Contains all functions to capture the azimuth. It supports a
 * 				quadrature encoder with two channels and an index every turn.
 * 				The azimuth could be read and a set position triggers an interrupt.
 * 				Data are readable if the quadrature encoder is calibrated by an
 * 				index pulse. The module notice if increments were loosing and calls
 * 				a hook function.
 * @{
 */

#ifndef BSP_QUADENC_H_
#define BSP_QUADENC_H_

#include "bsp.h"


/*
 * ----------------------------------------------------------------------------
 * Type declarations
 * ----------------------------------------------------------------------------
 */

/**
 * \typedef	bsp_quadenccallback_t
 * \brief	Interrupt callback function called at a set azimuth.
 */
typedef void (*bsp_quadenccallback_t)(uint32_t azimuth);


/*
 * ----------------------------------------------------------------------------
 * Configurations
 * ----------------------------------------------------------------------------
 */
#define BSP_QUADENC_INC_PER_TURN	(2000-1)	/*!< Number of increments each turn. */
#define BSP_QUADENC_ROTERROR_HOOK	0			/*!< Enable or disable the rotation hook function bsp_QuadencRoterrorHook() */


/*
 * ----------------------------------------------------------------------------
 * Hardware configurations
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Quadrature input channel A.
 */
static const bsp_gpioconf_t BSP_QUADENC_INCA = {
		RCC_AHB1Periph_GPIOE, GPIOE, GPIO_Pin_9, GPIO_Mode_AF, GPIO_PuPd_UP, GPIO_AF_TIM1
};

/**
 * \brief	Quadrature input channel B.
 */
static const bsp_gpioconf_t BSP_QUADENC_INCB = {
		RCC_AHB1Periph_GPIOE, GPIOE, GPIO_Pin_11, GPIO_Mode_AF, GPIO_PuPd_UP, GPIO_AF_TIM1
};

/**
 * \brief	Quadrature input channel I (index).
 */
static const bsp_gpioconf_t BSP_QUADENC_INCI = {
		RCC_AHB1Periph_GPIOE, GPIOE, GPIO_Pin_2, GPIO_Mode_IN, GPIO_PuPd_NOPULL
};

#define BSP_QUADENC_TIMER			TIM1					/*!< Port base address of the timer port */
#define BSP_QUADENC_TIMER_PERIPH	RCC_APB2Periph_TIM1		/*!< RCC AHB peripheral of the timer port */
#define BSP_QUADENC_POS_CHANEL		CHANNEL3					/*!< Capture compare channel to generate the position interrupt */

/* Interrupt settings */
#define BSP_QUADENC_POS_IRQ_CHANEL		TIM1_CC_IRQn		/*!< NVIC timer interrupt (capture compare) */
#define BSP_QUADENC_POS_IRQ_SOURCE		TIM_IT_CC3			/*!< NVIC timer source */
#define BSP_QUADENC_POS_IRQ_PRIORITY	3					/*!< NVIC timer interrupt priority */
#define BSP_QUADENC_POS_IRQ_Handler		TIM1_CC_IRQHandler	/*!< NVIC timer handler */

#define BSP_QUADENC_I_IRQ_CHANEL		EXTI2_IRQn			/*!< NVIC GPIO interrupt */
#define BSP_QUADENC_I_IRQ_PRIORITY		8					/*!< NVIC GPIO interrupt priority */
#define BSP_QUADENC_I_IRQ_Handler		EXTI2_IRQHandler	/*!< NVIC GPIO handler */


/*
 * ----------------------------------------------------------------------------
 * Prototypes
 * ----------------------------------------------------------------------------
 */
extern void bsp_QuadencInit(void);
extern uint8_t bsp_QuadencGet(uint32_t *azimuth);
extern void bsp_QuadencSetCapture(uint32_t azimuth);
extern void bsp_QuadencPosCallback(bsp_quadenccallback_t callback);


#endif /* BSP_QUADENC_H_ */

/**
 * @}
 */

/**
 * @}
 */

