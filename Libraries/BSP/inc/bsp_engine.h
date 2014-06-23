/**
 * \file		bsp_engine.h
 * \brief		Supports the full-bridge DC engine driver.
 * \date		2014-05-26
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	bsp
 * @{
 *
 * \addtogroup	bsp_engine
 * \brief		This Module provides the functions to configure the engine
 * 				speed and the rotating direction. The driver is set into direct
 * 				PWM mode and supports both directions. The speed is adjustable
 * 				over the duty cycle of the PWM signal.
 * 				When either the thermal shutdown or overcurrent protection circuit
 * 				is activated, the ALERT output goes high (CMOS output).
 * @{
 */

#ifndef BSP_ENGINE_H_
#define BSP_ENGINE_H_

#include "bsp.h"


/*
 * ----------------------------------------------------------------------------
 * Engine settings
 * ----------------------------------------------------------------------------
 */

/** PWM frequency of the laser pulse generator. */
#define BSP_ENGINE_PWM_FREQ			84000000
/** Period register of the PWM. The frequency of the laser pulse replay is f = BSP_ENGINE_PWM_FREQ[Hz] / (BSP_ENGINE_PWM_PERIOD-1)  */
#define BSP_ENGINE_PWM_PERIOD		4201


/*
 * ----------------------------------------------------------------------------
 * Hardware configurations
 * ----------------------------------------------------------------------------
 */

/** Hardware label of alert input (active high). */
static const bsp_gpioconf_t BSP_ENGINE_ALERT_PORT = {
		RCC_AHB1Periph_GPIOA, GPIOA, GPIO_Pin_15, GPIO_Mode_IN, GPIO_PuPd_UP
};

/** Hardware label of the configuration pin IN1. */
static const bsp_gpioconf_t BSP_ENGINE_IN1_PORT = {
		RCC_AHB1Periph_GPIOA, GPIOA, GPIO_Pin_8, GPIO_Mode_OUT, GPIO_PuPd_DOWN
};

/** Hardware label of the configuration pin IN2. */
static const bsp_gpioconf_t BSP_ENGINE_IN2_PORT = {
		RCC_AHB1Periph_GPIOA, GPIOA, GPIO_Pin_2, GPIO_Mode_OUT, GPIO_PuPd_DOWN
};

/** Hardware label of the standby input (active low). */
static const bsp_gpioconf_t BSP_ENGINE_STANDBY_PORT = {
		RCC_AHB1Periph_GPIOA, GPIOA, GPIO_Pin_3, GPIO_Mode_OUT, GPIO_PuPd_DOWN
};

/** Hardware label of the PWM output pin, which sets the engine speed. */
static const bsp_gpioconf_t BSP_ENGINE_PWM_PORT = {
		RCC_AHB1Periph_GPIOA, GPIOA, GPIO_Pin_1, GPIO_Mode_AF, GPIO_PuPd_DOWN, GPIO_AF_TIM5
};

/** RCC AHB peripheral of the timer port. */
#define BSP_ENGINE_TIMER_PORT_PERIPH	RCC_APB1Periph_TIM5
/** Port base address of the timer port */
#define BSP_ENGINE_TIMER_PORT_BASE		TIM5
/** Used PWM channel */
#define BSP_ENGINE_TIMER_PORT_CHANEL	CHANNEL2


/*
 * ----------------------------------------------------------------------------
 * Driver settings
 * ----------------------------------------------------------------------------
 */
#define BSP_ENGINE_CCW		((uint8_t) 0)		/*!< Engine rotating direction counterclockwise. */
#define BSP_ENGINE_CW		((uint8_t) 1)		/*!< Engine rotating direction clockwise. */

#define BSP_ENGINE_IS(direction) (((direction) == BSP_ENGINE_CCW) || ((direction) == BSP_ENGINE_CW))


/*
 * ----------------------------------------------------------------------------
 * Prototypes
 * ----------------------------------------------------------------------------
 */
extern void bsp_EngineInit(void);
extern void bsp_EngineEnalble(void);
extern void bsp_EngineDisable(void);
extern void bsp_EngineSpeed(int32_t speed);
extern uint8_t bsp_EngineAlert(void);


#endif /* BSP_ENGINE_H_ */

/**
 * @}
 */

/**
 * @}
 */
