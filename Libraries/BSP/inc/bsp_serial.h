/**
 * \file		bsp_serial.h
 * \brief		Board support package for the user communication over UART.
 * \date		2014-05-03
 * \version		0.2
 * \author		Kevin Gerber
 *
 * \addtogroup	bsp
 * @{
 *
 * \addtogroup	bsp_serial
 * \brief		The BSP_SERIAL module provides a function to initialize the UART
 * 				port for the user communication. The UART will be configured in
 * 				interrupt mode.
 * 				This module uses a circular buffer to manage the data. There are
 * 				two functions to fill the transmission circular buffer and one
 * 				function to read form the receive buffer. All functions are non
 * 				blocked!
 * @{
 */

#ifndef BSP_SERIAL_H_
#define BSP_SERIAL_H_

#include "bsp.h"

/*
 * ----------------------------------------------------------------------------
 * Buffer configurations
 * ----------------------------------------------------------------------------
 */
#define	TX_BUFFER_LEN		(1<<8)		/*!< TX buffer storage size */
#define	RX_BUFFER_LEN		(1<<8)		/*!< RX buffer storage size */


/*
 * ----------------------------------------------------------------------------
 * Hardware configurations
 * ----------------------------------------------------------------------------
 */

/** Hardware label from the UART RX pin */
static const bsp_gpioconf_t BSP_SERIAL_RX = {
		RCC_AHB1Periph_GPIOD, GPIOD, GPIO_Pin_9, GPIO_Mode_AF, GPIO_PuPd_UP, GPIO_AF_USART3
};

/** Hardware label from the UART TX pin */
static const bsp_gpioconf_t BSP_SERIAL_TX = {
		RCC_AHB1Periph_GPIOD, GPIOD, GPIO_Pin_8, GPIO_Mode_AF, GPIO_PuPd_UP, GPIO_AF_USART3
};

#define BSP_SERIAL_PORT			USART3					/*!< Port base address of the UART port */
#define BSP_SERIAL_PERIPH		RCC_APB1Periph_USART3	/*!< RCC AHB peripheral of the UART port */

#define BSP_SERIAL_IRQ_CHANEL	USART3_IRQn			/*!< NVIC UART interrupt */
#define BSP_SERIAL_IRQ_PRIORITY	3					/*!< NVIC UART interrupt priority */
#define BSP_SERIAL_IRQ_Handler	USART3_IRQHandler	/*!< NVIC interrupt handler */

/* UART settings */
#define BSP_SERIAL_UART_BAUD	115200				/*!< UART baud */
#define BSP_SERIAL_UART_LENGTH	USART_WordLength_8b	/*!< UART word length */
#define BSP_SERIAL_UART_STOP	USART_StopBits_1	/*!< UART number of stop bits */
#define BSP_SERIAL_UART_PARITY	USART_Parity_No		/*!< UART parity bit */
#define BSP_SERIAL_UART_FLOW	USART_HardwareFlowControl_None	/*!< UART hardware flow control */


/*
 * ----------------------------------------------------------------------------
 * Function prototypes
 * ----------------------------------------------------------------------------
 */
extern void bsp_SerialInit(void);
extern uint8_t bsp_SerialCharPut(char a);
extern uint8_t bsp_SerialCharGet(char *a);
extern uint32_t bsp_SerialStringPut(char *string, uint32_t length);

#endif /* BSP_SERIAL_H_ */

/**
 * @}
 */

/**
 * @}
 */
