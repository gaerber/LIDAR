/**
 * \file		bsp_engine.c
 * \brief		Supports the full-bridge DC engine driver.
 * \date		2014-05-26
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	bsp
 * @{
 *
 * \addtogroup	bsp_engine
 * @{
 */

#include "bsp_engine.h"

/*
 * ----------------------------------------------------------------------------
 * Private functions prototypes
 * ----------------------------------------------------------------------------
 */
void bsp_EngineSpeedSet(uint32_t speed);
void bsp_EngineDirection(uint8_t direction);


/*
 * ----------------------------------------------------------------------------
 * Implementation
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Initialize the engine driver IC.
 */
void bsp_EngineInit(void) {
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;

	uint16_t PrescalerValue;

	/* Initialize all GPIOs in their functions */
	bsp_GpioInit(&BSP_ENGINE_ALERT_PORT);
	bsp_GpioInit(&BSP_ENGINE_IN1_PORT);
	bsp_GpioInit(&BSP_ENGINE_IN2_PORT);
	bsp_GpioInit(&BSP_ENGINE_STANDBY_PORT);
	bsp_GpioInit(&BSP_ENGINE_PWM_PORT);

	/* Engine in standby mode */
	GPIO_ResetBits(BSP_ENGINE_STANDBY_PORT.base, BSP_ENGINE_STANDBY_PORT.pin);

	/* Sets the engine rotation clockwise as default */
	bsp_EngineDirection(BSP_ENGINE_CW);

	/* --- Initialize PWM unit ------------------------ */

	/* TIM clock enable */
	RCC_APB1PeriphClockCmd(BSP_ENGINE_TIMER_PORT_PERIPH, ENABLE);

	/* Compute the prescaler value */
	PrescalerValue = (uint16_t) ((SystemCoreClock /2) / BSP_ENGINE_PWM_FREQ) - 1;

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = BSP_ENGINE_PWM_PERIOD;
	TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(BSP_ENGINE_TIMER_PORT_BASE, &TIM_TimeBaseStructure);

	/* PWM Mode configuration */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	/* Check the PWM port */
	switch (BSP_ENGINE_TIMER_PORT_CHANEL) {
	case CHANNEL1:
		/* PWM Mode configuration: Channel 1 */
		TIM_OC1Init(BSP_ENGINE_TIMER_PORT_BASE, &TIM_OCInitStructure);
		break;

	case CHANNEL2:
		/* PWM Mode configuration: Channel 2 */
		TIM_OC2Init(BSP_ENGINE_TIMER_PORT_BASE, &TIM_OCInitStructure);
		TIM_OC2PreloadConfig(BSP_ENGINE_TIMER_PORT_BASE, TIM_OCPreload_Enable);
		break;

	case CHANNEL3:
		/* PWM Mode configuration: Channel 3 */
		TIM_OC3Init(BSP_ENGINE_TIMER_PORT_BASE, &TIM_OCInitStructure);
		break;

	case CHANNEL4:
		/* PWM Mode configuration: Channel 4 */
		TIM_OC4Init(BSP_ENGINE_TIMER_PORT_BASE, &TIM_OCInitStructure);
		break;

	default:
		assert(BSP_ENGINE_TIMER_PORT_CHANEL);
		break;
	}

	/* Enables  TIMx peripheral preload register on ARR */
	TIM_ARRPreloadConfig(BSP_ENGINE_TIMER_PORT_BASE, ENABLE);
}

/**
 * \brief	Enable the engine. It will turn with the configured speed.
 */
void bsp_EngineEnalble(void) {
	/* Enable PWM counter */
	TIM_Cmd(BSP_ENGINE_TIMER_PORT_BASE, ENABLE);

	/* Engine out of standby mode */
	GPIO_SetBits(BSP_ENGINE_STANDBY_PORT.base, BSP_ENGINE_STANDBY_PORT.pin);
}

/**
 * \brief	Disable the engine. It will be stopped.
 */
void bsp_EngineDisable(void) {
	/* Engine in standby mode */
	GPIO_ResetBits(BSP_ENGINE_STANDBY_PORT.base, BSP_ENGINE_STANDBY_PORT.pin);

	/* Disable PWM counter */
	TIM_Cmd(BSP_ENGINE_TIMER_PORT_BASE, DISABLE);
}

/**
 * \brief	Sets the engine speed and the rotation direction.
 * \param[in]	speed is the new speed of the engine. Positive values turn
 * 				clockwise, negative values turn the engine counterclockwise.
 */
void bsp_EngineSpeed(int32_t speed) {
	/* The last direction is used due to performance */
	static uint8_t direction = BSP_ENGINE_CW;

	/* Check the direction */
	if (speed < 0) {
		/* Is a direction change necessary? */
		if (direction != BSP_ENGINE_CCW) {
			/* Change the rotation direction */
			bsp_EngineDirection(BSP_ENGINE_CCW);
			direction = BSP_ENGINE_CCW;
		}
		/* Sets the new speed */
		bsp_EngineSpeedSet(-1*speed);
	}
	else {
		/* Is a direction change necessary? */
		if (direction != BSP_ENGINE_CW) {
			/* Change the rotation direction */
			bsp_EngineDirection(BSP_ENGINE_CW);
			direction = BSP_ENGINE_CW;
		}
		/* Sets the new speed */
		bsp_EngineSpeedSet(speed);
	}
}

/**
 * \brief	Changes the engine speed by update the duty cycle of the PWM output.
 * \param[in]	speed	New speed of the engine. Equivalent to the duty cycle of the
 * 				PWM. It must be smaller than the period register, which is configured
 * 				in BSP_ENGINE_PWM_PERIOD.
 */
void bsp_EngineSpeedSet(uint32_t speed) {
	switch (BSP_ENGINE_TIMER_PORT_CHANEL) {
	case CHANNEL1:
		/* Set the Capture Compare Register: Channel 1 */
		TIM_SetCompare1(BSP_ENGINE_TIMER_PORT_BASE, speed % BSP_ENGINE_PWM_PERIOD);
		break;

	case CHANNEL2:
		/* Set the Capture Compare Register: Channel 2 */
		TIM_SetCompare2(BSP_ENGINE_TIMER_PORT_BASE, speed % BSP_ENGINE_PWM_PERIOD);
		break;

	case CHANNEL3:
		/* Set the Capture Compare Register: Channel 3 */
		TIM_SetCompare3(BSP_ENGINE_TIMER_PORT_BASE, speed % BSP_ENGINE_PWM_PERIOD);
		break;

	case CHANNEL4:
		/* Set the Capture Compare Register: Channel 4 */
		TIM_SetCompare4(BSP_ENGINE_TIMER_PORT_BASE, speed % BSP_ENGINE_PWM_PERIOD);
		break;

	default:
		assert(BSP_ENGINE_TIMER_PORT_CHANEL);
		break;
	}
}

/**
 * \brief	Sets the engine rotating direction.
 * \param[in]	direction is the new engine direction.
 * 			\arg BSP_ENGINE_CCW
 * 			\arg BSP_ENGINE_CW
 */
void bsp_EngineDirection(uint8_t direction) {
	assert(BSP_ENGINE_IS(direction));

	/* Always change the low pin first, so it works over the breaks */
	if (direction == BSP_ENGINE_CCW) {
		GPIO_SetBits(BSP_ENGINE_IN2_PORT.base, BSP_ENGINE_IN2_PORT.pin);
		GPIO_ResetBits(BSP_ENGINE_IN1_PORT.base, BSP_ENGINE_IN1_PORT.pin);
	}
	else {
		GPIO_SetBits(BSP_ENGINE_IN1_PORT.base, BSP_ENGINE_IN1_PORT.pin);
		GPIO_ResetBits(BSP_ENGINE_IN2_PORT.base, BSP_ENGINE_IN2_PORT.pin);
	}
}

/**
 * \brief	Read the alert input.
 * \return	FALSE if an alert occurs. An alert could be a thermal shutdown or overcurrent.
 */
uint8_t bsp_EngineAlert(void) {
	return GPIO_ReadInputDataBit(BSP_ENGINE_ALERT_PORT.base, BSP_ENGINE_ALERT_PORT.pin);
}



/**
 * @}
 */

/**
 * @}
 */
