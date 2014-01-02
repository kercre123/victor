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
#include <moviVectorUtils.h>

/// Function that reads 8 ushort values into a ushort8 vector from LUT
/// @param[in] ushort8 in_values ushort8 vector type to read input for performing LUT
/// @param[in] u8* lut_memory pointer to the LUT memory
/// @param[out] ushort8 returned vectorized LUT seek results
ushort8 svuGet16BitVals16BitLUT(ushort8 in_values, u16* lut_memory);

/// Function that reads 8 uchar values into a ushort8 vector from LUT
/// @param[in] ushort8 in_values ushort8 vector type to read input for performing LUT
/// @param[in] u8* lut_memory pointer to the LUT memory
/// @param[out] uchar8 returned vectorized LUT seek results
uchar8 svuGet8BitVals16BitLUT(ushort8 in_values, u8* lut_memory);

/// Function that reads 8 uchar values into a uchar8 vector from LUT
/// @param[in] uchar8 in_values ushort8 vector type to read input for performing LUT
/// @param[in] u8* lut_memory pointer to the LUT memory
/// @param[out] uchar8 returned vectorized LUT seek results
uchar8 svuGet8BitVals8BitLUT(uchar8 in_values, u8* lut_memory);

/// Function that reads 16 uchar values into a uchar16 vector from LUT
/// @param[in] uchar16 in_values uchar16 vector type to read input for performing LUT
/// @param[in] u8* lut_memory pointer to the LUT memory
/// @param[out] uchar16 returned vectorized LUT seek results
uchar16 svuGet16_8BitVals8BitLUT(uchar16 in_values, u8* lut_memory);

#endif //__SVUCOMMONSHAVELUT__H_
