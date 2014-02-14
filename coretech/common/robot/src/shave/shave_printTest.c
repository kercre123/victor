/**
File: shave_printTest.c
Author: Peter Barnum
Created: 2013

Just printf a test string from the Shave side

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/config.h"

#ifdef USING_MOVIDIUS_SHAVE_COMPILER

void PrintTest()
{
  printf("Shave printf test passed\n");

  __asm("BRU.SWIH 0"); // We're finished, so tell the shave to halt
}

#else

#include "anki/common/robot/utilities_c.h"

void emulate_PrintTest()
{
  printf("Shave printf test is not running on the Shave\n");
}

#endif
