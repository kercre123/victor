///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Simple Pseudo Random Number Library
/// 
/// Allows for painting memory with a known pseudo random pattern
/// Or verifying memory against the same known pattern
/// @note This is NOT a cryptographically secure PRNG generator. This is a
/// Linear Congruential Generator (LCG).
/// See: http://en.wikipedia.org/wiki/Linear_congruential_generator
/// The magic values used here are from Donald Knuth's MMIX LCG.

// 1: Includes
// ----------------------------------------------------------------------------
#include <mv_types.h>
#include <assert.h>
#include <swcRandom.h>

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------
#define DEFAULT_INITIAL_STATE  (1ULL)

#define MAGIC_VALUE_a  (6364136223846793005ULL)
#define MAGIC_VALUE_c  (1442695040888963407ULL)

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data 
// ----------------------------------------------------------------------------
static u64 localRandState = DEFAULT_INITIAL_STATE;

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------

void swcRandInit(u64 initValue)
{
    localRandState = initValue;
}

u64 swcRandGetRandValue(void)
{
    return swcRandGetRandValue_r(&localRandState);
}

u64 swcRandGetRandValue_r(u64 *seed) {
    u64 ret = MAGIC_VALUE_a * (*seed) + MAGIC_VALUE_c;
    *seed = ret;
    return ret;
}

int swcRandBufferOp(tyRandOperation operation, void * targetAddress, u32 len, u64 seed)
{
    int retVal = 0;
    u8  * pByte;
    u32 * pWord;
    u32 i;
    pByte = targetAddress;
    pWord = targetAddress;
    switch (operation)
    {
    case RAND_WRITE:
        for (i=0;i<len;i++)
        {
            pByte[i] = (u8)swcRandGetRandValue_r(&seed); // We just take the bottom byte each time for simplicity
        }
        break;
    case RAND_VERIFY:
        for (i=0;i<len;i++)
        {
            if (pByte[i] != (u8)swcRandGetRandValue_r(&seed))
                retVal++;
        }
        break;
    case RAND_WRITE_32:
        for (i=0;i<(len>>2);i++)
        {
            pWord[i] = (u32)swcRandGetRandValue_r(&seed); // We just take the bottom byte each time for simplicity
        }
        break;
    case RAND_VERIFY_32:
        for (i=0;i<(len>>2);i++)
        {
            if (pWord[i] != (u32)swcRandGetRandValue_r(&seed))
                retVal++;
        }
        break;
    default:
        assert(FALSE);
        break;
    }

    return retVal;
}
