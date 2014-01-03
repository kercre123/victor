/**
File: shave_addVectors.c
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/config.h"

#ifdef USING_MOVIDIUS_SHAVE_COMPILER
#define INNER_LOOP_VERSION 1 // Change this to another number as desired
#else
#define INNER_LOOP_VERSION 1 // This must always be 1 on the PC or Leon
#endif

void addVectors(
  const s32 * restrict in1,
  const s32 * restrict in2,
  s32 * restrict out,
  const s32 numElements
  )
{
#if INNER_LOOP_VERSION == 1

#elif INNER_LOOP_VERSION == 2

#endif
}