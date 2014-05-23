/**
 * \file		bsp_quadenc.c
 * \brief		Supports all functions to capture the azimuth.
 * \date		2014-05-20
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	bsp
 * @{
 *
 * \addtogroup	bsp_quadenc
 * @{
 */

#include "bsp_quadenc.h"


#if BSP_QUADENC_ROTERROR_HOOK
extern void bsp_QuadencRoterrorHook(void);
#endif


/*
 * ----------------------------------------------------------------------------
 * Local variables
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Calibration flag. It is 1 if the absolute position is dictated.
 */
static uint8_t g_calibration = 0;

/**
 * \brief	User defined position callback function. Execute in a interrupt.
 */
static bsp_quadenccallback_t g_pos_callback = NULL;


/*
 * -----------------------------------------------------------------------
 * Interrupt functions
 * -----------------------------------------------------------------------
 */

/**
 * \brief	Caption compare interrupt handler.
 */
void BSP_QUADENC_POS_IRQ_Handler(void) {
	uint32_t incs;

	/* Capture compare interrupt on selected channel */
	if (TIM_GetITStatus(BSP_QUADENC_TIMER, BSP_QUADENC_POS_IRQ_SOURCE) != RESET) {
		TIM_ClearITPendingBit(BSP_QUADENC_TIMER, BSP_QUADENC_POS_IRQ_SOURCE);
		/* Check if a callback is registered */
		if (g_pos_callback != NULL) {
			/* Reads the effective position */
			if (bsp_QuadencGet(&incs)) {
				/* Execute callback function */
				g_pos_callback(incs);
			}
		}
	}
}

/**
 * \brief	Index interrupt handler. The position must be set back to zero.
 */
void BSP_QUADENC_I_IRQ_Handler(void) {
	uint32_t incs;

	/* Rising edge interrupt of the index */
	if(EXTI_GetITStatus(BSP_QUADENC_INCI.pin) != RESET) {
		/* Clear the interrupt flag */
		EXTI_ClearITPendingBit(BSP_QUADENC_INCI.pin);

#if BSP_QUADENC_ROTERROR_HOOK
		/* Check rotation increments */
		if (bsp_QuadencGet(&incs) && incs != 0) {
			/* Rotation error detected */
			bsp_QuadencRoterrorHook();
		}
#endif

		/* Calibrate the zero angle */
		TIM_SetCounter(BSP_QUADENC_TIMER, 0);
		g_calibration = 1;
	}
}


/*
 * ----------------------------------------------------------------------------
 * Implementation
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Initialize the interface of the quadrature encoder.
 */
void bsp_QuadencInit(void) {
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* Initialize all GPIOs in their function */
	bsp_GpioInit(&BSP_QUADENC_INCA);
	bsp_GpioInit(&BSP_QUADENC_INCB);
	bsp_GpioInit(&BSP_QUADENC_INCI);

	/* Timer peripheral clock enable */
	RCC_APB2PeriphClockCmd(BSP_QUADENC_TIMER_PERIPH, ENABLE);

	/* Configure the quadrature encoder */
	TIM_EncoderInterfaceConfig(BSP_QUADENC_TIMER, TIM_EncoderMode_TI12,
			TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = BSP_QUADENC_INC_PER_TURN;
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(BSP_QUADENC_TIMER, &TIM_TimeBaseStructure);

	/* Configure automatic reload register */
	TIM_ARRPreloadConfig(BSP_QUADENC_TIMER, ENABLE);
	TIM_SetAutoreload(BSP_QUADENC_TIMER, BSP_QUADENC_INC_PER_TURN);

	TIM_CtrlPWMOutputs(BSP_QUADENC_TIMER, ENABLE);

	/* --- Position interrupt configuration ------------ */

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Disable;
	TIM_OCInitStructure.TIM_Pulse = 0xFFFF;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	/* Check the caption compare channel */
	switch (BSP_QUADENC_POS_CHANEL) {
	case CHANEL1:
		/* PWM Mode configuration: Channel1 */
		TIM_OC1Init(BSP_QUADENC_TIMER, &TIM_OCInitStructure);
		break;

	case CHANEL2:
		/* PWM Mode configuration: Channel2 */
		TIM_OC2Init(BSP_QUADENC_TIMER, &TIM_OCInitStructure);
		break;

	case CHANEL3:
		/* PWM Mode configuration: Channel3 */
		TIM_OC3Init(BSP_QUADENC_TIMER, &TIM_OCInitStructure);
		break;

	case CHANEL4:
		/* PWM Mode configuration: Channel4 */
		TIM_OC4Init(BSP_QUADENC_TIMER, &TIM_OCInitStructure);
		break;

	default:
		assert(BSP_QUADENC_TIMER);
		break;
	}

	/* Enable timer interrupt on capture compare */
	TIM_ITConfig(BSP_QUADENC_TIMER, BSP_QUADENC_POS_IRQ_SOURCE, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = BSP_QUADENC_POS_IRQ_CHANEL;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = BSP_QUADENC_POS_IRQ_PRIORITY;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* --- GPIO interrupt for index -------------------- */

	/* Enable SYSCFG clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	/* Connect EXTI to the GPIO pin */
	SYSCFG_EXTILineConfig(BSP_GPIO_TO_EXTIPORT(BSP_QUADENC_INCI.base),
			BSP_GPIO_PIN_TO_SOURCE(BSP_QUADENC_INCI.pin));

	/* Configure EXTI line */
	EXTI_InitStructure.EXTI_Line = BSP_QUADENC_INCI.pin;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	/* Enable and set the Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = BSP_QUADENC_I_IRQ_CHANEL;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = BSP_QUADENC_I_IRQ_PRIORITY;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Sets the default parameter value */
	g_calibration = 0;
	g_pos_callback = NULL;

	/* Enable timer */
	TIM_Cmd(BSP_QUADENC_TIMER, ENABLE);
}

/**
 * \brief	Reads the current value of the azimuth.
 * \param[out]	azimuth is the current value of the azimuth. Azimuth in degree
 * 				is this value divided by BSP_QUADENC_INC_PER_TURN.
 * \return	FLASE if the quadrature encoder is not calibrated yet. The read
 * 			value is probably incorrect.
 */
uint8_t bsp_QuadencGet(uint32_t *azimuth) {
	/* Gets the counter value */
	*azimuth = TIM_GetCounter(BSP_QUADENC_TIMER);

	return g_calibration;
}

/**
 * \brief	Sets the next azimuth position. When this position is reached, the
 * 			interrupt occurs and execute the registered callback function.
 */
void bsp_QuadencSetCapture(uint32_t azimuth) {
	/* Check the caption compare channel */
	switch (BSP_QUADENC_POS_CHANEL) {
	case CHANEL1:
		/* Set caption compare register: Channel 1 */
		TIM_SetCompare1(BSP_QUADENC_TIMER, azimuth);
		break;

	case CHANEL2:
		/* Set caption compare register: Channel 2 */
		TIM_SetCompare2(BSP_QUADENC_TIMER, azimuth);
		break;

	case CHANEL3:
		/* Set caption compare register: Channel 3 */
		TIM_SetCompare3(BSP_QUADENC_TIMER, azimuth);
		break;

	case CHANEL4:
		/* Set caption compare register: Channel 4 */
		TIM_SetCompare4(BSP_QUADENC_TIMER, azimuth);
		break;

	default:
		assert(BSP_QUADENC_TIMER);
		break;
	}
}

/**
 * \brief	Sets the user defined callback function. It will be called
 * 			if an position interrupt occurs.
 * \param[in] int_callback Function pointer to the user defined callback
 * 			function. NULL if no callback is used.
 */
void bsp_QuadencPosCallback(bsp_quadenccallback_t int_callback) {
	g_pos_callback = int_callback;
}


/**
 * @}
 */

/**
 * @}
 */
