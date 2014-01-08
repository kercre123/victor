/**
File: shave_addVectors.c
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/config.h"

staticInline void AddVectors_s32x4_NaturalC(
  const s32 * restrict pIn1,
  const s32 * restrict pIn2,
  s32 * restrict pOut,
  const s32 numElements
  )
{
  s32 i;
  for(i=0; i<numElements-3; i+=4) {
    pOut[i]   = pIn1[i]   + pIn2[i];
    pOut[i+1] = pIn1[i+1] + pIn2[i+1];
    pOut[i+2] = pIn1[i+2] + pIn2[i+2];
    pOut[i+3] = pIn1[i+3] + pIn2[i+3];
  }
}

#ifdef USING_MOVIDIUS_SHAVE_COMPILER

#define OPTIMIZATION_VERSION 1 // Change this to another number as desired
#define FASTEST_VERSION 2 // The version of the loop that runs the fastest

#if OPTIMIZATION_VERSION != FASTEST_VERSION
#warning Current inner loop version is not the fastest version
#endif

void AddVectors_s32x4(
  const s32 * restrict pIn1,
  const s32 * restrict pIn2,
  s32 * restrict pOut,
  const s32 numElements
  )
{
#if OPTIMIZATION_VERSION == 1
  // Takes about .00020 seconds for 3000 elements
  AddVectors_s32x4_NaturalC(pIn1, pIn2, pOut, numElements);

#elif OPTIMIZATION_VERSION == 2
  // Takes about .00011 seconds for 3000 elements

  int4* pIn1x4 = (int4*)pIn1;
  int4* pIn2x4 = (int4*)pIn2;
  int4* pOutx4 = (int4*)pOut;

  const s32 numElements4 = numElements >> 2;

  s32 i;
  for(i=0; i<numElements4; i++) {
    pOutx4[i] = pIn1x4[i] + pIn2x4[i];
  }

#endif

  __asm("BRU.SWIH 0"); // We're finished, so tell the shave to halt
}

#else // #ifdef USING_MOVIDIUS_SHAVE_COMPILER

void emulate_AddVectors_s32x4(
  const s32 * restrict pIn1,
  const s32 * restrict pIn2,
  s32 * restrict pOut,
  const s32 numElements
  )
{
  AddVectors_s32x4_NaturalC(pIn1, pIn2, pOut, numElements);
}

#endif // #ifdef USING_MOVIDIUS_SHAVE_COMPILER ... #else