/**
 * \file		bsp_serial.c
 * \brief		Board support package for the user communication over UART.
 * \date		2014-05-03
 * \version		0.2
 * \author		Kevin Gerber
 *
 * \note		Read and write pointer overflow tested.
 *
 * \addtogroup	bsp
 * @{
 *
 * \addtogroup	bsp_serial
 * @{
 */

#include "bsp.h"
#include "bsp_serial.h"


/*
 * ----------------------------------------------------------------------------
 * Private data types
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Circular buffer structure.
 */
typedef struct {
	uint32_t tx_read;				/*!< TX buffer start index (reading) */
	uint32_t tx_write;				/*!< TX Buffer end index (writing) */
	char tx_buffer[TX_BUFFER_LEN];	/*!< TX buffer storage */
	uint8_t tx_sending;				/*!< TX transmission is pending */
	uint32_t rx_read;				/*!< RX buffer start index (reading) */
	uint32_t rx_write;				/*!< RX buffer end index (writing) */
	char rx_buffer[RX_BUFFER_LEN];	/*!< RX buffer storage */
} circbuff_t;


/*
 * ----------------------------------------------------------------------------
 * Private functions prototypes
 * ----------------------------------------------------------------------------
 */
void bsp_SerialIrqTxHandler(void);
void bsp_SerialIrqRxHandler(void);
void bsp_SerialSend(uint16_t data);
void bsp_SerialReceive(uint16_t *data);
void bsp_SerialTxIrqEnable(void);
void bsp_SerialTxIrqDisable(void);


/*
 * -----------------------------------------------------------------------
 * Private variables
 * -----------------------------------------------------------------------
 */

/**
 * \brief	Circular buffer manager.
 */
static circbuff_t g_CircularBuffer;


/*
 * -----------------------------------------------------------------------
 * Interrupt functions
 * -----------------------------------------------------------------------
 */

/**
 * \brief	This function handles USARTx global interrupt request.
 */
void BSP_SERIAL_IRQ_Handler(void) {
	/* UART receive data register not empty */
	if(USART_GetITStatus(BSP_SERIAL_PORT, USART_IT_RXNE) != RESET) {
		bsp_SerialIrqRxHandler();
	}

	/* UART transmit data register is empty */
	if(USART_GetITStatus(BSP_SERIAL_PORT, USART_IT_TXE) != RESET) {
		bsp_SerialIrqTxHandler();
	}
}

/**
 * \brief	UART TX interrupt handler. It will be called by BSP_SERIAL_IRQ_Handler().
 */
void bsp_SerialIrqTxHandler(void) {
	/* Check if character are available to send */
	if (g_CircularBuffer.tx_read != g_CircularBuffer.tx_write) {
		/* Send the next character */
		bsp_SerialSend((uint16_t) g_CircularBuffer.tx_buffer[g_CircularBuffer.tx_read++
		                                                     & (TX_BUFFER_LEN-1)]);
		g_CircularBuffer.tx_sending = 1;
	}
	else {
		/* There are no more character to send */
		g_CircularBuffer.tx_sending = 0;
		/* Disable the TX interrupt */
		bsp_SerialTxIrqDisable();
	}
}

/**
 * \brief	UART RX interrupt handler. It will be called by BSP_SERIAL_IRQ_Handler().
 */
void bsp_SerialIrqRxHandler(void) {
	uint16_t c;

	/* Get the received character from the hardware */
	bsp_SerialReceive(&c);

	/* Check if the RX circular buffer is not full */
	if (g_CircularBuffer.rx_read + RX_BUFFER_LEN != g_CircularBuffer.rx_write) {
		/* Store the incoming character in the circular buffer */
		g_CircularBuffer.rx_buffer[g_CircularBuffer.rx_write++ & (RX_BUFFER_LEN-1)] = (char) c;
	}
	/* Else: If no space is available, the character will be lost */
}


/*
 * ----------------------------------------------------------------------------
 * Implementation
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Initialize the UART for the user communication.
 * 			The UART will be configured with the following settings:
 * 			- 115200 baud
 * 			- 8 data bit
 * 			- one stop bit
 * 			- without a parity check bit
 * 			- without flow control
 * 			- RX/TX interrupts
 * 			.
 * 			This configuration is set in the header file bsp_serial.h.
 */
void bsp_SerialInit(void) {
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* Initialize all GPIOs in their function */
	bsp_GpioInit(&BSP_SERIAL_RX);
	bsp_GpioInit(&BSP_SERIAL_TX);

	/* UART Peripheral clock enable */
	if (BSP_SERIAL_PERIPH == RCC_APB2Periph_USART1 || BSP_SERIAL_PERIPH == RCC_APB2Periph_USART6) {
		RCC_APB2PeriphClockCmd(BSP_SERIAL_PERIPH, ENABLE);
	}
	else {
		RCC_APB1PeriphClockCmd(BSP_SERIAL_PERIPH, ENABLE);
	}

	/* Enable the USART OverSampling by 8 */
	USART_OverSampling8Cmd(BSP_SERIAL_PORT, ENABLE);

	/* Initialize the USART */
	USART_InitStructure.USART_BaudRate = BSP_SERIAL_UART_BAUD;
	USART_InitStructure.USART_WordLength = BSP_SERIAL_UART_LENGTH;
	USART_InitStructure.USART_StopBits = BSP_SERIAL_UART_STOP;
	USART_InitStructure.USART_Parity = BSP_SERIAL_UART_PARITY;
	USART_InitStructure.USART_HardwareFlowControl = BSP_SERIAL_UART_FLOW;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(BSP_SERIAL_PORT, &USART_InitStructure);

	/* Enable the USART interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = BSP_SERIAL_IRQ_CHANEL;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = BSP_SERIAL_IRQ_PRIORITY;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Enable the UART transmit interrupt: this interrupt is generated when
	 * the UART transmit data register is empty.
	 * This is disabled until a string must be send. */
	USART_ITConfig(BSP_SERIAL_PORT, USART_IT_TXE, DISABLE);

	/* Enable the UART receive interrupt: this interrupt is generated when
	 * the UART receive data register not empty. */
	USART_ITConfig(BSP_SERIAL_PORT, USART_IT_RXNE, ENABLE);

	/* Enable UART */
	USART_Cmd(BSP_SERIAL_PORT, ENABLE);

	/* Reset the circular buffer */
	g_CircularBuffer.rx_read = g_CircularBuffer.rx_write;
	g_CircularBuffer.tx_read = g_CircularBuffer.tx_write;
}

/**
 * \brief	Puts a character into the circular buffer.
 * \param[in]	a is the character, which will put into the circular buffer.
 * \return	True if the character was put into the circular buffer, otherwise false.
 */
uint8_t bsp_SerialCharPut(char a) {
	uint8_t success = 0;

	/* Check if space is available in the circular buffer */
	if (g_CircularBuffer.tx_read + (TX_BUFFER_LEN - 2) != g_CircularBuffer.tx_write) {

		/* Put the character into the circular buffer */
		g_CircularBuffer.tx_buffer[g_CircularBuffer.tx_write++ & (TX_BUFFER_LEN-1)] = a;

		/* If the circular buffer is not sending, the TX interrupt must be enabled
		 * to start the transmission */
		if (g_CircularBuffer.tx_sending == 0) {
			/* Enable the TX interrupt */
			bsp_SerialTxIrqEnable();
		}
		success = 1;
	}

	return success;
}

/**
 * \brief	Reads a character from the circular buffer and gives it to the user.
 * \param[out]	a Reference to the character storage.
 * \return	False if no character is available in the circular buffer.
 */
uint8_t bsp_SerialCharGet(char *a) {
	uint8_t success = 0;

	/* Checks if a character is available */
	if (g_CircularBuffer.rx_read != g_CircularBuffer.rx_write) {
		/* Gets the next character */
		*a = g_CircularBuffer.rx_buffer[g_CircularBuffer.rx_read++ & (RX_BUFFER_LEN-1)];
		success = 1;
	}

	return success;
}

/**
 * \brief	Puts a string into the circular buffer.
 * \param[in]	string is a pointer of the char array.
 * \param[in]	length is the length of the string, who will be sent.
 * \return	The number of character, which were placed successfully in the circular buffer.
 */
uint32_t bsp_SerialStringPut(char *string, uint32_t length) {
	uint8_t success = 1;
	uint32_t sendet_char;

	for (sendet_char=0; success==1 && sendet_char<length; sendet_char++) {
		success = bsp_SerialCharPut(string[sendet_char]);
	}

	/* Correction if no space was available */
	if (success == 0) {
		sendet_char--;
	}

	return sendet_char;
}

/**
 * \brief		Transmit a single byte over the UART.
 * \param[in]	data Byte to transmit.
 */
void bsp_SerialSend(uint16_t data) {
	USART_SendData(BSP_SERIAL_PORT, (data&0xFF));
}

/**
 * \brief		Receive a single byte from the UART.
 * \param[out]	data Received byte.
 */
void bsp_SerialReceive(uint16_t *data) {
	*data = (USART_ReceiveData(BSP_SERIAL_PORT) & 0xFF);
}

/**
 * \brief	Enable the TX interrupt.
 * 			This function is used, if chars are available to send.
 */
void bsp_SerialTxIrqEnable(void) {
	USART_ITConfig(BSP_SERIAL_PORT, USART_IT_TXE, ENABLE);
}

/**
 * \brief	Disable the TX interrupt.
 * 			This function is used, if no sending chars are available.
 */
void bsp_SerialTxIrqDisable(void) {
	USART_ITConfig(BSP_SERIAL_PORT, USART_IT_TXE, DISABLE);
}

/**
 * @}
 */

/**
 * @}
 */
