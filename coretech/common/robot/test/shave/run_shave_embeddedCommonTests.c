/**
File: run_shave_embeddedCommonTests.cpp
Author: Peter Barnum
Created: 2013

Code to run the shave parts of the unit tests in embeddedCommonTests.cpp on a Myriad.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "run_shave_embeddedCommonTests.h"

volatile s32 whichTest;
char buffer[MAX_SHAVE_BYTES];

// Test 1: GTEST_TEST(CoreTech_Common, SimpleShaveTest)
const s32 * restrict in1;
const s32 * restrict in2;
s32 * restrict out;
const s32 numElements;

int main()
{
  if(whichTest == 0) {
    printf("Shave Test 0 passed\n");
  } else if(whichTest == 1) {
  } else {
    printf("Shave Error: Unknown test %d\n", (int)whichTest);
  }
} // void main() 