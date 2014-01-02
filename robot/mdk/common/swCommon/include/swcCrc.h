///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     Simple Table based CRC Calculation library 
/// 
/// 
/// 
/// 
/// 

#ifndef SWC_CRC_H
#define SWC_CRC_H 

// 1: Includes
// ----------------------------------------------------------------------------
#include "mv_types.h"
#include <swcLeonUtils.h>

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

/// Calculate simple CRC32 over a byte buffer of byteLength
/// @param[in] pBuffer byte pointer to buffer 
/// @param[in] byteLength length of buffer in bytes
/// @param[in] pt initial endianness of the buffer
/// @return    32 bit crc of the buffer
u32 swcCalcCrc32(u8 *pBuffer, u32 byteLength, pointer_type pt);

#endif // SWC_CRC_H  

