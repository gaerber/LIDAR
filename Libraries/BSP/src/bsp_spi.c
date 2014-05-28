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
 * @{
 */

#include "bsp.h"
#include "bsp_spi.h"


/*
 * ----------------------------------------------------------------------------
 * Private functions prototypes
 * ----------------------------------------------------------------------------
 */
void bsp_SPIChipSelect(bsp_spics_t chip);
void bsp_SPIChipDeselect(bsp_spics_t chip);
void bsp_SPISendByte(uint8_t data);
void bsp_SPIReceiveByte(uint8_t *data);


/*
 * ----------------------------------------------------------------------------
 * Implementation
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Initialize the SPI interface in master mode. Clock speed is set to 10.5MHz.
 */
void bsp_SPIInit(void) {
	uint32_t i;
	SPI_InitTypeDef SPI_InitStructure;

	/* Initialize all registred chip selects */
	for (i=0; i < BSP_SPI_CS_ELEMENTCTR; i++) {
		/* Initialize all GPIOs in their function */
		bsp_GpioInit(&(BSP_SPI_CS[i]));

		/* Chip must be deselected */
		bsp_SPIChipDeselect(i);
	}

	/* SPI GPIO configuration */
	for (i=0; i<3; i++) {
		/* Initialize all GPIOs in their function */
		bsp_GpioInit(&(BSP_SPI_PORT_LABEL[i]));
	}

	/* Enable the SPI clock */
	if (BSP_SPI_PERIPH == RCC_APB1Periph_SPI2 || BSP_SPI_PERIPH == RCC_APB1Periph_SPI3) {
		RCC_APB1PeriphClockCmd(BSP_SPI_PERIPH, ENABLE);
	}
	else {
		RCC_APB2PeriphClockCmd(BSP_SPI_PERIPH, ENABLE);
	}

	/* SPI configuration as master */
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;	/* Falling clock edge */
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;	/* SPI_BaudRatePrescaler_8 -> 10.5MHz */
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(BSP_SPI_PORT, &SPI_InitStructure);

	/* Enable the SPI peripheral */
	SPI_Cmd(BSP_SPI_PORT, ENABLE);
}

/**
 * \brief	Select a chip.
 */
void bsp_SPIChipSelect(bsp_spics_t chip) {
	GPIO_ResetBits(BSP_SPI_CS[chip].base, BSP_SPI_CS[chip].pin);
}

/**
 * \brief	Deselect a chip.
 */
void bsp_SPIChipDeselect(bsp_spics_t chip) {
	GPIO_SetBits(BSP_SPI_CS[chip].base, BSP_SPI_CS[chip].pin);
}


/**
 * \brief		Transmit a single byte over the SPI.
 * \param[in]	data Byte to transmit.
 */
void bsp_SPISendByte(uint8_t data) {
	SPI_I2S_SendData(BSP_SPI_PORT, (uint16_t) data);
}

/**
 * \brief		Receive a single byte from the SPI.
 * \param[out]	data Received byte.
 */
void bsp_SPIReceiveByte(uint8_t *data) {
	*data = (uint8_t) (SPI_I2S_ReceiveData(BSP_SPI_PORT) & 0xFF);
}

/**
 * \brief	Send a data block over SPI to a chip.
 * 			This is an non blocking function. The data were buffered and the transfer
 * 			will be in interrupt mode.
 * \param[in] 	chip A reference to the chip, which will be selected during the transmission.
 * \param[in]	tx_data Array of data, to transmit.
 * \param[in]	len Length of the data array. Maximum length is defined in BSP_SPI_BUFSIZE_DATA.
 * \param[out]	rx_data Data storage of the received data.
 * \return	TRUE if transmission successes, otherwise FALSE.
 */
uint8_t bsp_SPITransmitBlocked(bsp_spics_t chip, const uint8_t *tx_data, uint8_t len, uint8_t *rx_data) {
	uint8_t success = 0;
	uint32_t timeout = 0x03FF;
	uint32_t ctr;

	/* Parameter check */
	assert(chip < BSP_SPI_CS_ELEMENTCTR);
	assert(tx_data);
	assert(len > 0);

	/* Select the chip */
	bsp_SPIChipSelect(chip);

	for (ctr=0; ctr<len; ctr++) {
		/* Send one byte */
		bsp_SPISendByte(tx_data[ctr]);

		while (SPI_I2S_GetFlagStatus(BSP_SPI_PORT, SPI_I2S_FLAG_TXE)==RESET && timeout>0) {
			/* wait until transmit complete */
			timeout--;
		}
		while (SPI_I2S_GetFlagStatus(BSP_SPI_PORT, SPI_I2S_FLAG_RXNE)==RESET && timeout>0) {
			/* wait until receive complete */
			timeout--;
		}

		while (SPI_I2S_GetFlagStatus(BSP_SPI_PORT, SPI_I2S_FLAG_BSY)==SET && timeout>0) {
			/* wait until SPI is not busy anymore */
			timeout--;
		}

		if (timeout > 0) {
			if (rx_data != NULL)
			/* Receive the incoming byte */
			bsp_SPIReceiveByte(&(rx_data[ctr]));
			success = 1;
		}
	}

	/* Transmission finished, deselect the chip */
	bsp_SPIChipDeselect(chip);

	return success;
}

/**
 * @}
 */

/**
 * @}
 */
