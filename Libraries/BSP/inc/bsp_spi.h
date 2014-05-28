/**
 * \file		bsp_spi.h
 * \brief		Supports all SPI for the communication.
 * \date		2014-05-12
 * \version		0.3
 * \author		Kevin Gerber
 *
 * \addtogroup	bsp
 * @{
 *
 * \addtogroup	bsp_spi
 * \brief		Supports all functions for the communication with the SPI interface.
 * 				It supports multiple chips over the same SPI interface. The controller
 * 				works as a master. Reading from the SPI bus works with call back functions.
 * @{
 */

#ifndef BSP_SPI_H_
#define BSP_SPI_H_

#include "bsp.h"


/*
 * ----------------------------------------------------------------------------
 * Hardware configurations
 * ----------------------------------------------------------------------------
 */

/**
 * \brief		List off all chip selects, which are supported by the SPI interface.
 * \warning		All chip selects require an entry in the BSP_SPI_CS array with
 * 				the correct hardware labels.
 */
typedef enum {
	BSP_SPI_CS_GP22 = 0,	/*!< Chip select from the TDC-GP22. */
	BSP_SPI_CS_ELEMENTCTR	/*!< Counts the chip selects. It must be the last element. */
} bsp_spics_t;

/**
 * \brief	Array off all used chip selects with their correct hardware label.
 */
static const bsp_gpioconf_t BSP_SPI_CS[] = {
	{RCC_AHB1Periph_GPIOA, GPIOA, GPIO_Pin_4, GPIO_Mode_OUT, GPIO_PuPd_UP},	/* BSP_SPI_CS_GP22 */
};

/**
 * \brief	Hardware label of the SPI interface.
 */
static const bsp_gpioconf_t BSP_SPI_PORT_LABEL[] = {
	/* SPI CLK */
	{ RCC_AHB1Periph_GPIOA, GPIOA, GPIO_Pin_5, GPIO_Mode_AF, GPIO_PuPd_NOPULL, GPIO_AF_SPI1 },
	/* SPI MISO */
	{ RCC_AHB1Periph_GPIOA, GPIOA, GPIO_Pin_6, GPIO_Mode_AF, GPIO_PuPd_NOPULL, GPIO_AF_SPI1 },
	/* SPI MOSI */
	{ RCC_AHB1Periph_GPIOB, GPIOB, GPIO_Pin_5, GPIO_Mode_AF, GPIO_PuPd_NOPULL, GPIO_AF_SPI1 }
};

#define BSP_SPI_PORT		SPI1					/*!< Port base address of the SPI port */
#define BSP_SPI_PERIPH		RCC_APB2Periph_SPI1		/*!< RCC AHB peripheral of the SPI port */


/*
 * ----------------------------------------------------------------------------
 * Function prototypes
 * ----------------------------------------------------------------------------
 */
extern void bsp_SPIInit(void);
extern uint8_t bsp_SPITransmitBlocked(bsp_spics_t chip, const uint8_t *tx_data, uint8_t len,
		uint8_t *rx_data);

#endif /* BSP_GP22_H_ */

/**
 * @}
 */

/**
 * @}
 */
