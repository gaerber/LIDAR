/**
 * \file		bsp_laser.c
 * \brief		Supports the laser pulse generator.
 * \date		2014-05-13
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	bsp
 * @{
 *
 * \addtogroup	bsp_laser
 * @{
 */

#include "bsp_laser.h"


/*
 * ----------------------------------------------------------------------------
 * Private functions prototypes
 * ----------------------------------------------------------------------------
 */
void bsp_LaserEnable(void);
void bsp_LaserDisable(void);


/*
 * ----------------------------------------------------------------------------
 * Private variables
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Counter of the remaining laser pulses.
 */
static uint32_t g_laserPulseCtr = 0;


/*
 * ----------------------------------------------------------------------------
 * Interrupt functions
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Timer interrupt handler. It is triggered by the capture compare register.
 */
void BSP_LASER_IRQ_Handler(void) {
	if (TIM_GetITStatus(BSP_LASER_TIMER_PORT_BASE, BSP_LASER_IRQ_SOURCE) != RESET) {
		TIM_ClearITPendingBit(BSP_LASER_TIMER_PORT_BASE, BSP_LASER_IRQ_SOURCE);
		/* One pulse generated */
		g_laserPulseCtr--;
		if (g_laserPulseCtr == 0) {
			/* All laser pulse were made */
			bsp_LaserDisable();
		}
	}
}


/*
 * ----------------------------------------------------------------------------
 * Implementation
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Initialize the laser pulse generator. It is used one PWM channel to make the pulse.
 */
void bsp_LaserInit(void) {
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	uint16_t PrescalerValue;

	/* Initialize all GPIOs in their function */
	bsp_GpioInit(&BSP_LASER_PORT);

	/* TIM clock enable */
	RCC_APB1PeriphClockCmd(BSP_LASER_TIMER_PORT_PERIPH, ENABLE);

	/* Compute the prescaler value */
	PrescalerValue = (uint16_t) ((SystemCoreClock /2) / BSP_LASER_FREQ) - 1;

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = BSP_LASER_PERIOD;
	TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(BSP_LASER_TIMER_PORT_BASE, &TIM_TimeBaseStructure);

	/* PWM Mode configuration */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;//BSP_LASER_PULSE_WIDTH;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	/* Check the PWM port */
	switch (BSP_LASER_TIMER_PORT_CHANEL) {
	case CHANEL1:
		/* PWM Mode configuration: Channel1 */
		TIM_OC1Init(BSP_LASER_TIMER_PORT_BASE, &TIM_OCInitStructure);
		TIM_OC1PreloadConfig(BSP_LASER_TIMER_PORT_BASE, TIM_OCPreload_Enable);
		break;

	case CHANEL2:
		/* PWM Mode configuration: Channel2 */
		TIM_OC2Init(BSP_LASER_TIMER_PORT_BASE, &TIM_OCInitStructure);
		TIM_OC2PreloadConfig(BSP_LASER_TIMER_PORT_BASE, TIM_OCPreload_Enable);
		break;

	case CHANEL3:
		/* PWM Mode configuration: Channel3 */
		TIM_OC3Init(BSP_LASER_TIMER_PORT_BASE, &TIM_OCInitStructure);
		TIM_OC3PreloadConfig(BSP_LASER_TIMER_PORT_BASE, TIM_OCPreload_Enable);
		break;

	case CHANEL4:
		/* PWM Mode configuration: Channel4 */
		TIM_OC4Init(BSP_LASER_TIMER_PORT_BASE, &TIM_OCInitStructure);
		TIM_OC4PreloadConfig(BSP_LASER_TIMER_PORT_BASE, TIM_OCPreload_Enable);
		break;

	default:
		assert(BSP_LASER_TIMER_PORT_CHANEL);
		break;
	}

	/* Enables  TIMx peripheral preload register on ARR */
	TIM_ARRPreloadConfig(BSP_LASER_TIMER_PORT_BASE, ENABLE);

	/* Enable timer interrupt on capture compare */
	TIM_ITConfig(BSP_LASER_TIMER_PORT_BASE, BSP_LASER_IRQ_SOURCE, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = BSP_LASER_IRQ_CHANEL;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = BSP_LASER_IRQ_PRIORITY;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Enable the timer */
	TIM_Cmd(BSP_LASER_TIMER_PORT_BASE, ENABLE);
}

/**
 * \brief	Generates a number of pulses in the configured repetition frequency and pulse width.
 * \param[in] nr_of_pulses is the number of pulse repetition.
 */
void bsp_LaserPulse(uint32_t nr_of_pulses) {
	g_laserPulseCtr = nr_of_pulses;
	bsp_LaserEnable();
}

/**
 * \brief	Enable the engine. It will turn with the configured speed.
 */
void bsp_LaserEnable(void) {
	/* Starts a new pulse */
	TIM_SetCounter(BSP_LASER_TIMER_PORT_BASE, BSP_LASER_PERIOD-20);
	/* Sets pulse width */
	TIM_SetCompare1(BSP_LASER_TIMER_PORT_BASE, BSP_LASER_PULSE_WIDTH);
}

/**
 * \brief	Disable the engine. It will be stopped.
 */
void bsp_LaserDisable(void) {
	/* Set output to low */
	TIM_SetCompare1(BSP_LASER_TIMER_PORT_BASE, 0);
}


/**
 * @}
 */

/**
 * @}
 */
