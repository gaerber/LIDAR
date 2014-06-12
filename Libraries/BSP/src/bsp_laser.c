/**
 * \file		bsp_laser.c
 * \brief		Supports the laser pulse generator.
 * \date		2014-05-26
 * \version		0.2
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
 * Local variables
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	User defined interrupt callback function called after a laser pulse
 * 			sequence.
 */
bsp_lasercallback_t g_int_callback = NULL;


/*
 * ----------------------------------------------------------------------------
 * Private functions prototypes
 * ----------------------------------------------------------------------------
 */
void bsp_LaserEnable(void);
void bsp_LaserDisable(void);


/*
 * ----------------------------------------------------------------------------
 * Interrupt functions
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Timer interrupt handler. It is triggered by the timer update flag.
 * 			This is set after the repetition of all pulses.
 */
void BSP_LASER_IRQ_Handler(void) {
	if (TIM_GetITStatus(BSP_LASER_TIMER_PORT_BASE, BSP_LASER_IRQ_SOURCE) != RESET) {
		TIM_ClearITPendingBit(BSP_LASER_TIMER_PORT_BASE, BSP_LASER_IRQ_SOURCE);
		/* All pulse were generated -> disable the generator */
		bsp_LaserDisable();
	}
}

/**
 * \brief	Software interrupt handler. A user callback function could be
 * 			registered with bsp_LaserSequenceCalback().
 */
void BSP_LASER_USR_IRQ_Handler(void) {
	/* Software interrupt */
    if(EXTI_GetITStatus(BSP_LASER_USR_IRQ_SOURCE) != RESET) {
        /* Clear the interrupt flag */
    	EXTI_ClearITPendingBit(BSP_LASER_USR_IRQ_SOURCE);
        /* Check if an user defined clallback function is set */
    	if (g_int_callback != NULL) {
        	/* Execute the user defined callback function */
    		g_int_callback();
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

	/* Initialize the overcurrent detection input */
	bsp_GpioInit(&BSP_LASER_NER_PORT);

	/* Initialize the laser output in his function */
	bsp_GpioInit(&BSP_LASER_PORT);

	/* TIM clock enable */
	RCC_APB2PeriphClockCmd(BSP_LASER_TIMER_PORT_PERIPH, ENABLE);

	/* Compute the prescaler value */
	PrescalerValue = (uint16_t) ((SystemCoreClock /2) / BSP_LASER_FREQ) - 1;

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = BSP_LASER_PERIOD;
	TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_CenterAligned1;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(BSP_LASER_TIMER_PORT_BASE, &TIM_TimeBaseStructure);

	/* PWM Mode configuration */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = BSP_LASER_PERIOD - BSP_LASER_PULSE_WIDTH;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
	/* Check the PWM port */
	switch (BSP_LASER_TIMER_PORT_CHANEL) {
	case CHANNEL1:
		/* PWM Mode configuration: Channel 1 */
		TIM_OC1Init(BSP_LASER_TIMER_PORT_BASE, &TIM_OCInitStructure);
		break;

	case CHANNEL2:
		/* PWM Mode configuration: Channel 2 */
		TIM_OC2Init(BSP_LASER_TIMER_PORT_BASE, &TIM_OCInitStructure);
		break;

	case CHANNEL3:
		/* PWM Mode configuration: Channel 3 */
		TIM_OC3Init(BSP_LASER_TIMER_PORT_BASE, &TIM_OCInitStructure);
		break;

	case CHANNEL4:
		/* PWM Mode configuration: Channel 4 */
		TIM_OC4Init(BSP_LASER_TIMER_PORT_BASE, &TIM_OCInitStructure);
		break;

	default:
		assert(BSP_LASER_TIMER_PORT_CHANEL);
		break;
	}

	/* Enable timer interrupt on capture compare */
	TIM_ITConfig(BSP_LASER_TIMER_PORT_BASE, BSP_LASER_IRQ_SOURCE, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = BSP_LASER_IRQ_CHANEL;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = BSP_LASER_IRQ_PRIORITY;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

/**
 * \brief	Registred an user defined callback function at the end of a laser
 * 			pulse sequence.
 */
void bsp_LaserSequenceCalback(bsp_lasercallback_t callback) {
	NVIC_InitTypeDef NVIC_InitStructure;

	/* Initialize the software interrupt */
	if (g_int_callback == NULL) {
		NVIC_InitStructure.NVIC_IRQChannel = BSP_LASER_USR_IRQ_CHANEL;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = BSP_LASER_USR_IRQ_PRIORITY;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
	}

	/* Registered the user defined callback function */
	g_int_callback = callback;
}

/**
 * \brief	Generates a number of pulses in the configured repetition frequency and pulse width.
 * \note	Direct access to the CMSIS, due to performance.
 * \param[in] nr_of_pulses is the number of pulse repetition.
 */
void bsp_LaserPulse(uint32_t nr_of_pulses) {
	/* parameter check */
	assert(nr_of_pulses);

	/* Sets the repetition counter */
	BSP_LASER_TIMER_PORT_BASE->RCR = 2 * nr_of_pulses - 1;
	/* Generate an update event to reload the repetition counter (only for TIM1 and TIM8) value immediately */
	BSP_LASER_TIMER_PORT_BASE->EGR = TIM_PSCReloadMode_Immediate;

	/* Starts the laser pulses */
	bsp_LaserEnable();
}

/**
 * \brief	Enable the laser pulse generator and send a sequence.
 */
void bsp_LaserEnable(void) {
	/* Starts a new pulse */
	TIM_SetCounter(BSP_LASER_TIMER_PORT_BASE, 0);

	/* Main output enable */
	TIM_CtrlPWMOutputs(BSP_LASER_TIMER_PORT_BASE, ENABLE);

	/* Enable the timer */
	TIM_Cmd(BSP_LASER_TIMER_PORT_BASE, ENABLE);
}

/**
 * \brief	Disable the laser pulse generator. Output has to be in low state.
 */
void bsp_LaserDisable(void) {
	/* Disable the timer */
	TIM_Cmd(BSP_LASER_TIMER_PORT_BASE, DISABLE);

	/* Disable the output to be sure it is low */
	TIM_CtrlPWMOutputs(BSP_LASER_TIMER_PORT_BASE, DISABLE);
}

/**
 * \brief	Read the overcurrent detection input.
 * \return	FALSE if an overcurrent is detected.
 */
uint8_t bsp_LaserOvercurrent(void) {
	return GPIO_ReadInputDataBit(BSP_LASER_NER_PORT.base, BSP_LASER_NER_PORT.pin);
}


/**
 * @}
 */

/**
 * @}
 */
