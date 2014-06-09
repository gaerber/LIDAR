/**
 * \file		data_encode.c
 * \brief		Encoder for the azimuth and the distance.
 * \date		2014-06-09
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	utility
 * @{
 */

#include <stdint.h>
#include "data_encode.h"


/*
 * ----------------------------------------------------------------------------
 * Implementation
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Encode the data (azimuth and distance). It is used a base64 encoding algorithms.
 * \param[in]	azimuth is the signed 12 bit azimuth value in tenth degree.
 * \param[in]	distance is the 12 bit unsigned distance value in millimeters.
 * \param[out]	base64 is a storage address of 4 bytes for the encoded data. MSB first.
 */
inline void dataEncode(int16_t azimuth, int16_t distance, char *base64) {
	/* Look up table due to performance */
	static const char look_up_table[] = {
			'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
			'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
			'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
			'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
			'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
	};

	/* Convert azimuth */
	base64[0] = look_up_table[(azimuth >> 6) & 0x3F];
	base64[1] = look_up_table[(azimuth) & 0x3F];

	/* Convert distance */
	base64[2] = look_up_table[(distance >> 6) & 0x3F];
	base64[3] = look_up_table[(distance) & 0x3F];
}

/**
 * @}
 */
