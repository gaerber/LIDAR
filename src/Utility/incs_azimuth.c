/**
 * \file		incs_azimuth.c
 * \brief		Convert the two types of the azimuth in each other.
 * \date		2014-06-09
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	utility
 * @{
 */

#include <stdint.h>
#include <math.h>

#include "incs_azimuth.h"
#include "bsp_quadenc.h"


/*
 * ----------------------------------------------------------------------------
 * Implementation
 * ----------------------------------------------------------------------------
 */

/**
 * \brief	Conversion of the increment value to the absolute azimuth.
 * \param[in]	increments is the value of the quadrature encoder.
 * \return	The absolute azimuth in tenth degrees.
 */
inline int16_t increments2tenthdegree(uint32_t increments) {
	return round(3600.0 / BSP_QUADENC_INC_PER_TURN * increments - 1800);
}

/**
 * \brief	Conversion of the absolute azimuth in increments of the quadrature
 * 			encoder.
 * \param[in]	tenthdegree is the absolute azimuth in tenth degree.
 * \return	Increments, based on the position value of the quadrature encoder.
 */
inline uint32_t tenthdegree2increments(int16_t tenthdegree) {
	return round((tenthdegree + 1800) / 3600.0 * BSP_QUADENC_INC_PER_TURN);
}

/**
 * \brief	Conversion of the relative azimuth or azimuth difference in increments
 * 			of the quadrature encoder.
 * \param[in]	tenthdegree azimuth difference in tenth degree.
 * \return	Relative increments based on the azimuth difference.
 */
inline uint32_t tenthdegree2increments_Relative(int16_t tenthdegree) {
	return round(tenthdegree / 3600.0 * BSP_QUADENC_INC_PER_TURN);
}

/**
 * @}
 */
