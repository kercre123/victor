///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief
///

#ifndef __SVUCOMMONSHAVELUT__H_
#define __SVUCOMMONSHAVELUT__H_

#include <mv_types.h>
#if !USE_OPENCL
#include <moviVectorUtils.h>
#endif

//!@{
/// Function that reads 8 ushort values into a ushort8 vector from LUT
/// @param[in] in_values ushort8 vector type to read input for performing LUT
/// @param[in] lut_memory pointer to the LUT memory
/// @return ushort8 LUT seek results
short8 svuGets16BitVals16BitLUT(short8 in_values, s16* lut_memory);
ushort8 svuGetu16BitVals16BitLUT(ushort8 in_values, u16* lut_memory);
//!@}

/// Function that reads 8 uchar values into a ushort8 vector from LUT
/// @param[in] in_values ushort8 vector type to read input for performing LUT
/// @param[in] lut_memory pointer to the LUT memory
/// @return uchar8 vectorized LUT seek results
uchar8 svuGet8BitVals16BitLUT(ushort8 in_values, u16* lut_memory);

/// Function that reads 8 uchar values into a uchar8 vector from LUT
/// @param[in] in_values ushort8 vector type to read input for performing LUT
/// @param[in] lut_memory pointer to the LUT memory
/// @return uchar8 vectorized LUT seek results
uchar8 svuGet8BitVals8BitLUT(uchar8 in_values, u8* lut_memory);

/// Function that reads 16 uchar values into a uchar16 vector from LUT
/// @param[in] in_values uchar16 vector type to read input for performing LUT
/// @param[in] lut_memory pointer to the LUT memory
/// @return uchar16 vectorized LUT seek results
uchar16 svuGet16_8BitVals8BitLUT(uchar16 in_values, u8* lut_memory);

#endif //__SVUCOMMONSHAVELUT__H_
