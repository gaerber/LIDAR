/**
 * \file		incs_azimuth.h
 * \brief		Convert the two types of the azimuth in each other.
 * \date		2014-06-09
 * \version		0.1
 * \author		Kevin Gerber
 *
 * \addtogroup	utility
 * @{
 */

#ifndef INCS_AZIMUTH_H_
#define INCS_AZIMUTH_H_


/*
 * ----------------------------------------------------------------------------
 * Prototypes
 * ----------------------------------------------------------------------------
 */
extern inline int16_t increments2tenthdegree(uint32_t increments);
extern inline uint32_t tenthdegree2increments(int16_t tenthdegree);
extern inline uint32_t tenthdegree2increments_Relative(int16_t tenthdegree);


#endif /* INCS_AZIMUTH_H_ */

/**
 * @}
 */
