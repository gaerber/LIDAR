/**
 * \file		data_encode.h
 * \brief		Encoder for the azimuth and the distance.
 * \date		2014-06-09
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	utility
 * \brief		Contains all utility function used by this project.
 * @{
 */

#ifndef DATA_ENCODE_H_
#define DATA_ENCODE_H_

/*
 * ----------------------------------------------------------------------------
 * Prototypes
 * ----------------------------------------------------------------------------
 */
extern inline void dataEncode(int16_t azimuth, int16_t distance, char *base64);


#endif /* DATA_ENCODE_H_ */

/**
 * @}
 */
