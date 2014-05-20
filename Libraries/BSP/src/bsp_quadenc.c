/**
 * \file		bsp_quadenc.h
 * \brief		Supports all functions to capture the angel.
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


/*
 * ----------------------------------------------------------------------------
 * Local variables
 * ----------------------------------------------------------------------------
 */

/** User defined position callback function. Execute in a interrupt. */
static bsp_quadenccallback_t g_pos_callback;


/*
 * ----------------------------------------------------------------------------
 * Private functions prototypes
 * ----------------------------------------------------------------------------
 */



/*
 * -----------------------------------------------------------------------
 * Interrupt functions
 * -----------------------------------------------------------------------
 */

/**
 * \brief	Caption compare interrupt handler.
 */
void BSP_QUADENC_POS_IRQ_Handler(void) {
	/* Capture compare interrupt on selected channel */
	if (TIM_GetITStatus(BSP_QUADENC_TIMER, BSP_QUADENC_POS_IRQ_SOURCE) != RESET) {
		TIM_ClearITPendingBit(BSP_QUADENC_TIMER, BSP_QUADENC_POS_IRQ_SOURCE);
		/* Check if a callback is registred */
		if (g_pos_callback != NULL) {
			/* Execute callback function */
			g_pos_callback();
		}
	}
}

/**
 * \brief	Index interrupt handler. The position must be set back to zero.
 */
void BSP_QUADENC_I_IRQ_Handler(void) {
	/* Rising edge interrupt of the index */
    if(EXTI_GetITStatus(BSP_QUADENC_INCI.pin) != RESET) {
        /* Clear the interrupt flag */
    	EXTI_ClearITPendingBit(BSP_QUADENC_INCI.pin);
    	/* Calibrate the zero angle */
    	TIM_SetCounter(BSP_QUADENC_TIMER, 0);
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

	/* Autoreload */
	TIM_SetAutoreload(BSP_QUADENC_TIMER, BSP_QUADENC_INC_PER_TURN);

	/* Enable timer interrupt on capture compare */
	TIM_ITConfig(BSP_QUADENC_TIMER, BSP_QUADENC_POS_IRQ_SOURCE, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = BSP_QUADENC_POS_IRQ_CHANEL;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = BSP_QUADENC_POS_IRQ_PRIORITY;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

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

	/* Enable timer */
	TIM_Cmd(BSP_QUADENC_TIMER, ENABLE);
}

/**
 * \brief	Reads the current value of the azimuth.
 * \return	Current azimuth.
 * \todo 	Prüfen ob index synchronisiert bevor rückgabe!!!
 */
uint32_t bsp_QuadencGet(void) {
	uint32_t azimuth;

	azimuth = TIM_GetCounter(BSP_QUADENC_TIMER);

	return azimuth;
}

/**
 * \brief	Sets the next azimuth position. When this position is reached, the
 * 			interrupt occurs and execute the registrated  callback function.
 * \todo	Dynamic compare channel.
 */
void bsp_QuadencSetCapture(uint32_t azimuth) {
	TIM_SetCompare1(BSP_QUADENC_TIMER, azimuth);
}

/**
 * \brief	Sets the user defined callback function. It will be called
 * 			if an position interrupt occurs.
 * \param[in] int_callback Function pointer to the user defined callback
 * 			function. NULL if no callback is used.
 */
void bsp_QuadencPosCallback(bsp_quadenccallback_t callback) {
	g_pos_callback = callback;
}


/**
 * @}
 */

/**
 * @}
 */
