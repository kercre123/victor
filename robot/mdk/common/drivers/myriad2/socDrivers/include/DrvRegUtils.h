///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     Macros and functions to ease register manipulation
/// 
/// 
/// 
/// 
/// 

#ifndef REG_UTILS_H
#define REG_UTILS_H 
#include "mv_types.h"
#include "DrvRegUtilsDefines.h"

// ----------------------------------------------------------------------------
// 1: Includes
// ----------------------------------------------------------------------------

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

/// Program a particular bitfield with a specific value
/// 
/// e.g. DrvRegSetBitField(0x70000000,4,5,0x13) would result in
/// 0x70000000 = 0xCAFE8938 where original value was 0xCAFE8888
/// @param[in] regAddress Address of register to modify
/// @param[in] startBit   Offset of the start of the bitfield to be modified
/// @param[in] numBits    Size of the bitfield in bits
/// @param[in] value      value to be written to the bitfield
/// @return    void
void DrvRegSetBitField(void * regAddress, u32 startBit, u32 numBits, u32 value);


/// Returns the value of a particular bitfield
/// 
/// @param[in] regAddress Address of register to read
/// @param[in] startBit   Offset of the start of the bitfield to be read
/// @param[in] numBits    Size of the bitfield in bits
/// @return    value of the bitfield
u32 DrvRegGetBitField(void * regAddress, u32 startBit, u32 numBits);

/// Poll waiting for any one of the bits in bitSetMask to be set
/// @param[in] Address of register to Poll
/// @param[in] 32 bit mask for bits to be tested
/// @param[in] Delay in micoseconds after which this function will timeout
/// @return    0 on success, non-zero on timeout
int DrvRegWaitForBitsSetAny(u32 registerAddress, u32 bitSetMask, u32 timeOutMicroSecs);

/// Poll waiting for all of the bits in bitSetMask to be set
/// @param[in] Address of register to Poll
/// @param[in] 32 bit mask for bits to be tested
/// @param[in] Delay in micoseconds after which this function will timeout
/// @return    0 on success, non-zero on timeout
int DrvRegWaitForBitsSetAll(u32 registerAddress, u32 bitSetMask, u32 timeOutMicroSecs);

/// Poll waiting for all of the bits in bitSetMask to be cleared
/// @param[in] Address of register to Poll
/// @param[in] 32 bit mask for bits to be tested
/// @param[in] Delay in micoseconds after which this function will timeout
/// @return    0 on success, non-zero on timeout
int DrvRegWaitForBitsClearAll(u32 registerAddress, u32 bitSetMask, u32 timeOutMicroSecs);

#endif // REG_UTILS_H  
