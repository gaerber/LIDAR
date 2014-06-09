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

inline int16_t increments2tenthdegree(uint32_t increments) {
	return round(3600.0 / BSP_QUADENC_INC_PER_TURN * increments - 1800);
}

inline uint32_t tenthdegree2increments(int16_t tenthdegree) {
	return round((tenthdegree + 1800) / 3600.0 * BSP_QUADENC_INC_PER_TURN);
}

inline uint32_t tenthdegree2increments_Relative(int16_t tenthdegree) {
	return round(tenthdegree / 3600.0 * BSP_QUADENC_INC_PER_TURN);
}

/**
 * @}
 */
