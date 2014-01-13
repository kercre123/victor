/**
File: shave_exampleTemplate.c
Author: Peter Barnum
Created: 2013

An example of how to create a shave kernel. Just copy-paste this code to create a new kernel.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/config.h"

staticInline void emulate_ExampleTemplate_NaturalC(
  const s32 parameter1
  )
{
  // Add non-shave-specific C code here
}

#ifdef USING_MOVIDIUS_SHAVE_COMPILER

#define OPTIMIZATION_VERSION 2 // Change this to another number as desired
#define FASTEST_VERSION 2 // The version of the loop that runs the fastest

#if OPTIMIZATION_VERSION != FASTEST_VERSION
#warning Current inner loop version is not the fastest version
#endif

void exampleTemplate(
  const s32 parameter1
  )
{
#if OPTIMIZATION_VERSION == 1

  ExampleTemplate_NaturalC(parameter1);

#elif OPTIMIZATION_VERSION == 2

  // Add code here

#endif

  __asm("BRU.SWIH 0"); // We're finished, so tell the shave to halt
}

#else // #ifdef USING_MOVIDIUS_SHAVE_COMPILER

void emulate_exampleTemplate(
  const s32 parameter1
  )
{
  emulate_ExampleTemplate_NaturalC(parameter1);
}

#endif // #ifdef USING_MOVIDIUS_SHAVE_COMPILER ... #else