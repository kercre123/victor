///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     API manipulating memory trabnsfers
///
///

#include "assert.h"
#include "mv_types.h"
#include "swcMemoryTransfer.h"
/// Function that r
/// @param[in] -
/// @param[out] -
//#########################################################
// Naive memcopy on LEON
//#########################################################
void swcU32memcpy(u32* dst, u32* src, u32 len)
{
  u32 i;

  // Assert aligned addresses
  assert( (  len      & 0x3) == 0 );
  assert( ( (u32)dst  & 0x3) == 0 );
  assert( ( (u32)src  & 0x3) == 0 );

  len = len>>2;
  for(i=0;i<len;i++)
   dst[i] = src[i];
}


//#########################################################
// u32 memset
//#########################################################
void swcU32memsetU32(u32 *addr, u32 lenB, u32 val)
{
  u32 i;
  lenB = lenB>>2;//div by 4 as we work on WORDS

  //Checks here...  could remove later...
  assert( (lenB&0x3 !=0) ||   (((u32)addr)&0x3 !=0));

  for(i=0;i<lenB;i++)
  {
    addr[i] = val;
  }
}

