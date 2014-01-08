/**
File: shave_addVectors.c
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/config.h"

#define INNER_LOOP_VERSION 1 // Change this to another number as desired (only makes a difference for Shave)
#define FASTEST_VERSION 2 // The version of the loop that runs the fastest

#if INNER_LOOP_VERSION != FASTEST_VERSION
#ifdef _MSC_VER
#pragma message ("Warning: Current inner loop version is not the fastest version")
#else
#warning Current inner loop version is not the fastest version
#endif
#endif

// INNER_LOOP_VERSION must always be 1 on the PC or Leon
#ifndef USING_MOVIDIUS_SHAVE_COMPILER
#undef INNER_LOOP_VERSION
#define INNER_LOOP_VERSION 1
#endif // #ifndef USING_MOVIDIUS_SHAVE_COMPILER

//#ifdef USING_MOVIDIUS_SHAVE_COMPILER
void AddVectors_s32x4(
  //#else
  //void emulate_AddVectors_s32x4(
  //#endif
  const s32 * restrict pIn1,
  const s32 * restrict pIn2,
  s32 * restrict pOut,
  const s32 numElements
  )
{
#if INNER_LOOP_VERSION == 1
  // Takes about .00020 seconds for 3000 elements

  s32 i;
  for(i=0; i<numElements-3; i+=4) {
    pOut[i]   = pIn1[i]   + pIn2[i];
    pOut[i+1] = pIn1[i+1] + pIn2[i+1];
    pOut[i+2] = pIn1[i+2] + pIn2[i+2];
    pOut[i+3] = pIn1[i+3] + pIn2[i+3];
  }

#elif INNER_LOOP_VERSION == 2
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

#ifdef USING_MOVIDIUS_SHAVE_COMPILER
  __asm(
  "BRU.SWIH 0"
    );
#endif
}