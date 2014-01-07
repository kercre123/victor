/**
File: run_shave_embeddedCommonTests.cpp
Author: Peter Barnum
Created: 2013

Code to run the shave parts of the unit tests in embeddedCommonTests.cpp on a Myriad.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "run_shave_embeddedCommonTests.h"
#include "anki/common/robot/shaveKernels_c.h"

volatile s32 whichTest;
char buffer[MAX_SHAVE_BYTES];

// Test 1: GTEST_TEST(CoreTech_Common, SimpleShaveTest)
const s32 * restrict pIn1;
const s32 * restrict pIn2;
s32 * restrict pOut;
s32 numElements;

int main()
{
  if(whichTest == 0) {
    printf("Shave Test 0 passed\n");
  } else if(whichTest == 1) {
    //printf("Shave: 0x%x=%d 0x%x=%d\n", &(pIn1[100]), pIn1[100], &(pIn2[303]), pIn2[303]);
    addVectors_s32x4(pIn1, pIn2, pOut, numElements);
  } else {
    printf("Shave Error: Unknown test %d\n", (int)whichTest);
  }
} // void main() 