/**
 * \file        bsp_led.h
 * \brief       Board support package to use all LEDs.
 * \date        2014-03-18
 * \version     0.2
 * \author		Kevin Gerber
 *
 * \note		The BSP_LED_OUT_3 is on PB8 and not on PA3, documented in the Carme manual.
 *
 * \addtogroup  bsp
 * @{
 *
 * \addtogroup  bsp_led
 * \brief		Support all LEDs as GPIO outputs. The LED could be set, reset and toggle.
 * @{
 */

#ifndef BSP_LED_H_
#define BSP_LED_H_

#include "bsp.h"


/*
 * ----------------------------------------------------------------------------
 * Hardware configurations
 * ----------------------------------------------------------------------------
 */

/**
 * \brief		List off all LEDs to use in software.
 * \warning		All LEDs require an entry in the BSP_LED_PORTS array with the correct hardware labels.
 */
typedef enum {
	BSP_LED_GREEN = 0,		/*!< The green LED identification number. */
	//BSP_LED_OUT_0,			/*!< Carme IO2 OUT0 LED */
	BSP_LED_OUT_1,			/*!< Carme IO2 OUT1 LED */
	BSP_LED_OUT_2,			/*!< Carme IO2 OUT2 LED */
	BSP_LED_OUT_3,			/*!< Carme IO2 OUT3 LED */
	BSP_LED_ELEMENTCTR		/*!< Counts the LEDs. It must be the last element. Do not use as LED! */
} bsp_led_t;

/**
 * \brief	Array off all used LED with their correct hardware label.
 */
static const bsp_gpioconf_t BSP_LED_PORTS[] = {
	{RCC_AHB1Periph_GPIOI, GPIOI, GPIO_Pin_6, GPIO_Mode_OUT, GPIO_PuPd_NOPULL},	/* BSP_LED_GREEN */
	//{RCC_AHB1Periph_GPIOA, GPIOA, GPIO_Pin_0, GPIO_Mode_OUT, GPIO_PuPd_NOPULL},	/* BSP_LED_OUT_0 */
	{RCC_AHB1Periph_GPIOH, GPIOH, GPIO_Pin_11, GPIO_Mode_OUT, GPIO_PuPd_NOPULL},	/* BSP_LED_OUT_1 */
	{RCC_AHB1Periph_GPIOH, GPIOH, GPIO_Pin_12, GPIO_Mode_OUT, GPIO_PuPd_NOPULL},	/* BSP_LED_OUT_2 */
	{RCC_AHB1Periph_GPIOB, GPIOB, GPIO_Pin_8, GPIO_Mode_OUT, GPIO_PuPd_NOPULL}	/* BSP_LED_OUT_3 */
};


/*
 * ----------------------------------------------------------------------------
 * Function prototypes
 * ----------------------------------------------------------------------------
 */
extern void bsp_LedInit(void);
extern void bsp_LedSetOn(bsp_led_t led);
extern void bsp_LedSetOff(bsp_led_t led);
extern void bsp_LedSetToggle(bsp_led_t led);

#endif /* BSP_LED_H_ */

/**
 * @}
 */

/**
 * @}
 */
