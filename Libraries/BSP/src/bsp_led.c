/**
 * \file        bsp_led.c
 * \brief       Board support package to use all the LEDs.
 * \date        2014-03-18
 * \version     0.2
 * \author		Kevin Gerber
 *
 * \addtogroup  bsp
 * @{
 *
 * \addtogroup  bsp_led
 * @{
 */

#include "bsp.h"
#include "bsp_led.h"


/*
 * ----------------------------------------------------------------------------
 * Implementation
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Initialize all LEDs on the board and sets them off as default.
 */
void bsp_LedInit(void) {
	bsp_led_t i_led;

	/* Initialize all registred LED from the port array. */
	for (i_led=0; i_led < BSP_LED_ELEMENTCTR; i_led++) {
		/* Initialize all GPIOs in their function */
		bsp_GpioInit(&(BSP_LED_PORTS[i_led]));

		/* Set the LEDs off as the default state. */
		bsp_LedSetOff(i_led);
	}
}

/**
 * \brief	Turns a LED on.
 * \param[in]	led The identification number of the LED, which turn on.
 */
void bsp_LedSetOn(bsp_led_t led) {
	assert(led < BSP_LED_ELEMENTCTR);

	/* Turns GPIO State high */
	GPIO_SetBits(BSP_LED_PORTS[led].base, BSP_LED_PORTS[led].pin);
}

/**
 * \brief	Turns a LED off.
 * \param[in]	led The identification number of the LED, which turn off.
 */
void bsp_LedSetOff(bsp_led_t led) {
	assert(led < BSP_LED_ELEMENTCTR);

	/* Turns GPIO State high */
	GPIO_ResetBits(BSP_LED_PORTS[led].base, BSP_LED_PORTS[led].pin);
}

/**
 * \brief	Toggles a LED.
 * \param[in]	led The identification number of the LED, which toggle.
 */
void bsp_LedSetToggle(bsp_led_t led) {
	assert(led < BSP_LED_ELEMENTCTR);

	/* Turns GPIO State high */
	GPIO_ToggleBits(BSP_LED_PORTS[led].base, BSP_LED_PORTS[led].pin);
}


/**
 * @}
 */

/**
 * @}
 */
