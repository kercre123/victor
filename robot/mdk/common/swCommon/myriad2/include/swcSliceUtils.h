///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     API manipulating Slice functionalities
///
///


#ifndef SWCSLICEUTILS_H_
#define SWCSLICEUTILS_H_

#include <swcDmaTypes.h>
#include <mv_types.h>


void swcDmaMemcpy(u32 svu_nr, u32 *dst , u32 *src, u32 len, u32 blocking);

#endif /* SWCSLICEUTILS_H_ */
