/**
 * \file		bsp_gp22.h
 * \brief		Supports all functions to manage the TDC GP22.
 * \date		2014-05-12
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	bsp
 * @{
 *
 * \addtogroup	bsp_gp22
 * @{
 */

#include "bsp.h"
#include "bsp_spi.h"
#include "bsp_gp22.h"

/*
 * ----------------------------------------------------------------------------
 * Local variables
 * ----------------------------------------------------------------------------
 */

/** User defined TDC-GP22 interrupt callback function */
static bsp_gp22callback_t g_int_callback;


/*
 * ----------------------------------------------------------------------------
 * Private functions prototypes
 * ----------------------------------------------------------------------------
 */
uint8_t bsp_GP22Configure(void);
uint32_t bytes2long(uint8_t *bytes);
uint16_t bytes2short(uint8_t *bytes);


/*
 * -----------------------------------------------------------------------
 * Interrupt functions
 * -----------------------------------------------------------------------
 */

/**
 * \brief	TDC-GP22 interrupt handler. Is called if data are available
 * 			from the TDC.
 */
void BSP_GP22_IRQ_Handler(void) {
	/* GP22 INT falling edge interrupt */
    if(EXTI_GetITStatus(BSP_GP22_INT.pin) != RESET) {
        /* Clear the interrupt flag */
    	EXTI_ClearITPendingBit(BSP_GP22_INT.pin);
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
 * \brief	Initialize the interface of the TDC-GP22 and configure the register.
 * 			Required the SPI interface and an interupt GPIO input.
 */
void bsp_GP22Init(void) {
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* Initialize the SPI */
	bsp_SPIInit();

	/* Enable the INT input */
	bsp_GpioInit(&BSP_GP22_INT);

	/* Enable SYSCFG clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	/* Connect EXTI to the GPIO pin */
	SYSCFG_EXTILineConfig(BSP_GPIO_TO_EXTIPORT(BSP_GP22_INT.base),
			BSP_GPIO_PIN_TO_SOURCE(BSP_GP22_INT.pin));

	/* Configure EXTI line */
	EXTI_InitStructure.EXTI_Line = BSP_GP22_INT.pin;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	/* Enable and set the Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = BSP_GP22_IRQ_CHANEL;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = BSP_GP22_IRQ_PRIORITY;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Reset the user defined callback function */
	g_int_callback = NULL;

	/* Configure the GP22 */
	bsp_GP22Configure();
}

/**
 * \brief	Sets the user defined callback function. It will be called
 * 			if an interrupt from the TDC occurs.
 * \param[in] int_callback Function pointer to the user defined callback
 * 			function. NULL if no callback is used.
 */
void bsp_GP22IntCallback(bsp_gp22callback_t int_callback) {
	g_int_callback = int_callback;
}

/**
 * \brief	Configure the registers of the GP22. This function makes the GP22 ready to use.
 * \return	FALSE if the SPI transmission get a time out.
 */
uint8_t bsp_GP22Configure(void) {
	uint8_t success = 1;

	/* Send opcode for the reset */
	success &= bsp_GP22SendOpcode(GP22_OP_Power_On_Reset);

	/* Set register 0 to 6 */
	success &= bsp_GP22RegWrite(GP22_WR_REG_0, BSP_GP22_REG0);
	success &= bsp_GP22RegWrite(GP22_WR_REG_1, BSP_GP22_REG1);
	success &= bsp_GP22RegWrite(GP22_WR_REG_2, BSP_GP22_REG2);
	success &= bsp_GP22RegWrite(GP22_WR_REG_3, BSP_GP22_REG3);
	success &= bsp_GP22RegWrite(GP22_WR_REG_4, BSP_GP22_REG4);
	success &= bsp_GP22RegWrite(GP22_WR_REG_5, BSP_GP22_REG5);
	success &= bsp_GP22RegWrite(GP22_WR_REG_6, BSP_GP22_REG6);

	return success;
}

/**
 * \brief	Sends an operation code to the TDC-GP22.
 * \param[in]	op is the operation code, which will be sent.
 * \return	FALSE if the SPI transmission get a time out.
 */
uint8_t bsp_GP22SendOpcode(uint8_t op) {
	uint8_t success;

	assert(GP22_IS_OP(op));

	/* Transmitting operation code */
	success = bsp_SPITransmitBlocked(BSP_SPI_CS_GP22, &op, 1, NULL);

	return success;
}

/**
 * \brief	Sets a Register of the TDC-GP22.
 * \param[in]	reg is the writable register of the GP22.
 * \param[in]	new_reg_val is the new register value.
 * \return	FALSE if the SPI transmission get a time out.
 */
uint8_t bsp_GP22RegWrite(uint8_t reg, uint32_t new_reg_val) {
	uint8_t success;
	uint8_t tx_bytes[5];

	assert(GP22_IS_WR(reg));

	/* Convert the data to an unsigned 8 bit integer */
	tx_bytes[0] = reg;
	tx_bytes[1] = (new_reg_val & 0xFF000000) >> 24;
	tx_bytes[2] = (new_reg_val & 0x00FF0000) >> 16;
	tx_bytes[3] = (new_reg_val & 0x0000FF00) >> 8;
	tx_bytes[4] = (new_reg_val & 0x000000FF) >> 0;

	/* Set the selected register */
	success = bsp_SPITransmitBlocked(BSP_SPI_CS_GP22, tx_bytes, 5, NULL);

	return success;
}

/**
 * \brief	Sets a Register of the TDC-GP22.
 * \param[in]	reg is the readable register of the GP22.
 * \param[out]	value is a pointer to the storage of the register value.
 * \param[in]	len indicates how many bytes have to read.
 * \return	FALSE if the SPI transmission get a time out.
 */
uint8_t bsp_GP22RegRead(uint8_t reg, uint32_t *value, uint8_t len) {
	uint8_t success;
	uint8_t tx_bytes[5];
	uint8_t rx_bytes[5];

	assert(GP22_IS_RD(reg));
	assert(len==2 || len==4);

	/* First send a read command */
	tx_bytes[0] = reg;
	success = bsp_SPITransmitBlocked(BSP_SPI_CS_GP22, tx_bytes, len+1, rx_bytes);

	/* Transform received bytes to a 32 bit integer */
	if (success) {
		if (len == 2) {
			*value = bytes2short(&(rx_bytes[1]));
		}
		else if (len == 4) {
			*value = bytes2long(&(rx_bytes[1]));
		}
		else {
			success = 0;
		}
	}

	return success;
}


/**
 * \brief	Converts a received byte array from the SPI interface to a unsigned 32 bit integer.
 * \param[in]	bytes Array with four bytes. MSB first.
 * \return	Converted unsigned 32 bit integer.
 */
uint32_t bytes2long(uint8_t *bytes) {
	uint32_t res_long;

	res_long = bytes[3];
	res_long |= bytes[2] << 8;
	res_long |= bytes[1] << 16;
	res_long |= bytes[0] << 24;

	return res_long;
}

/**
 * \brief	Converts a received byte array from the SPI interface to a unsigned 16 bit integer.
 * \param[in]	bytes Array with two bytes. MSB first.
 * \return	Converted unsigned 16 bit integer.
 */
uint16_t bytes2short(uint8_t *bytes) {
	uint16_t res_long;

	res_long = bytes[1];
	res_long |= bytes[0] << 8;

	return res_long;
}

/**
 * @}
 */

/**
 * @}
 */
