///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     API for Simple Pseudo Random Number Generator Library
/// 
/// Allows for painting memory with a known pseudo random pattern
/// Or verifying memory against the same known pattern
/// @note This is NOT a cryptographically secure PRNG generator. This is a
/// Linear Congruential Generator (LCG).
/// See: http://en.wikipedia.org/wiki/Linear_congruential_generator
/// The magic values used here are from Donald Knuth's MMIX LCG.

#ifndef SWC_RANDOM_H
#define SWC_RANDOM_H 

// 1: Includes
// ----------------------------------------------------------------------------
#include "swcRandomDefines.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

/// Reset the base seed of the PRNG.
/// This is equivalent to srand() from standard C.
///
/// @param[in] initValue 64 bit initial value
void swcRandInit(u64 initValue);

/// Get next 64 bit random value in sequence defined by
/// the global seed which was set using swcRandInit()
/// This is equivalent to rand() from standard C.
///
/// @return    64 bit pseudo random value between [0, RAND_MAX]
u64 swcRandGetRandValue(void);

/// Get next 64 bit random value in sequence defined by seed.
/// The result of this function does not depend on the global seed
/// that was set by swcRandInit. If it is called many times with
/// the parameter pointing to the same value, then the result will
/// be the same.
/// This is equivalent to rand_r() from standard C.
///
/// @param[in, out] seed pointer to the 64 bit seed value, which will be updated.
/// @return    64 bit pseudo random value between [0, RAND_MAX]
u64 swcRandGetRandValue_r(u64 *seed);

/// Paint or verify a buffer with pseudo random pattern 
///
/// Function which either paints a buffer with a pseudo random  
/// pattern, or verifies that buffer against an expected        
/// pseudo random pattern.                                      
/// The Seed for the random pattern is passed as a parameter    
///
/// @param[in] operation (RAND_WRITE, RAND_VERIFY, or 32 bit equivalents) 
/// @param[in] targetAddress buffer to be painted of verified (word or byte depending on operation) 
/// @param[in] len length of buffer in bytes
/// @param[in] seed Seed to be applied to the rand operation
/// @return    0 on success, non-zero otherwise  
int swcRandBufferOp(tyRandOperation operation, void * targetAddress, u32 len, u64 seed);

#endif


