/**
 * \file        bsp_gp22.c
 * \brief       Supports all functions to manage the TDC GP22.
 * \date        2014-05-12
 * \version     0.1
 * \author		Kevin Gerber
 *
 * \addtogroup  bsp
 * @{
 *
 * \addtogroup  bsp_gp22
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

/** TDC-GP22 register 0 configuration */
static const uint8_t g_register_0[] = {WR_REG_0, BSP_GP22_REG0};
/** TDC-GP22 register 1 configuration */
static const uint8_t g_register_1[] = {WR_REG_1, BSP_GP22_REG1};
/** TDC-GP22 register 2 configuration */
static const uint8_t g_register_2[] = {WR_REG_2, BSP_GP22_REG2};
/** TDC-GP22 register 3 configuration */
static const uint8_t g_register_3[] = {WR_REG_3, BSP_GP22_REG3};
/** TDC-GP22 register 4 configuration */
static const uint8_t g_register_4[] = {WR_REG_4, BSP_GP22_REG4};
/** TDC-GP22 register 5 configuration */
static const uint8_t g_register_5[] = {WR_REG_5, BSP_GP22_REG5};
/** TDC-GP22 register 6 configuration */
static const uint8_t g_register_6[] = {WR_REG_6, BSP_GP22_REG6};


/*
 * ----------------------------------------------------------------------------
 * Private functions prototypes
 * ----------------------------------------------------------------------------
 */
uint8_t bsp_GP22Configure(void);
uint32_t bytes2long(uint8_t *bytes);


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
        EXTI_ClearITPendingBit(BSP_GP22_INT.pin);
        if (g_int_callback != NULL) {
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
	success &= bsp_GP22Opcode(OP_Power_On_Reset);

	/* Set register 0 to 6 */
	success &= bsp_SPITransmitBlocked(BSP_SPI_CS_GP22, g_register_0, 5, NULL);
	success &= bsp_SPITransmitBlocked(BSP_SPI_CS_GP22, g_register_1, 5, NULL);
	success &= bsp_SPITransmitBlocked(BSP_SPI_CS_GP22, g_register_2, 5, NULL);
	success &= bsp_SPITransmitBlocked(BSP_SPI_CS_GP22, g_register_3, 5, NULL);
	success &= bsp_SPITransmitBlocked(BSP_SPI_CS_GP22, g_register_4, 5, NULL);
	success &= bsp_SPITransmitBlocked(BSP_SPI_CS_GP22, g_register_5, 5, NULL);
	success &= bsp_SPITransmitBlocked(BSP_SPI_CS_GP22, g_register_6, 5, NULL);

	return success;
}

/**
 * \brief	Sends an opcode to the TDC-GP22.
 * \param[in]	op is the opcode, which will be sent.
 * \return	True if the opcode was sent, otherwise FALSE.
 */
uint8_t bsp_GP22Opcode(gp22_opcode_t op) {
	uint8_t success = 0;
	uint8_t tx_byte;

	tx_byte = op;

	/* Transmitting */
	success = bsp_SPITransmitBlocked(BSP_SPI_CS_GP22, &tx_byte, 1, NULL);

	return success;
}

uint32_t bytes2long(uint8_t *bytes) {
	uint32_t res_long;

	res_long = bytes[3];
	res_long |= bytes[2] << 8;
	res_long |= bytes[1] << 16;
	res_long |= bytes[0] << 24;

	return res_long;
}

uint8_t bsp_GP22ReadState(uint32_t *stat) {
	uint8_t success = 0;
	uint8_t tx_bytes[5];
	uint8_t rx_bytes[5];

	/* Read from register stat */
	tx_bytes[0] = RD_STAT;
	success = bsp_SPITransmitBlocked(BSP_SPI_CS_GP22, tx_bytes, 5, rx_bytes);

	/* Transform received bytes to a 32 bit integer */
	if (success) {
		*stat = bytes2long(&(rx_bytes[1]));
	}

	return success;
}

uint8_t bsp_GP22ReadTimeDelay(uint32_t *result) {
	uint8_t success = 0;
	uint8_t tx_bytes[5];
	uint8_t rx_bytes[5];

	/* Read from register stat */
	tx_bytes[0] = RD_RES_3;
	success = bsp_SPITransmitBlocked(BSP_SPI_CS_GP22, tx_bytes, 5, rx_bytes);

	/* Transform received bytes to a 32 bit integer */
	if (success) {
		*result = bytes2long(&(rx_bytes[1]));
	}

	return success;
}

/**
 * @}
 */

/**
 * @}
 */
