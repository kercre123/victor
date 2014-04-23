/**
File: run_pc_embeddedTests.cpp
Author: Peter Barnum
Created: 2014-02-14

Basic unit test calling utility for common and vision tests

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef ROBOT_HARDWARE

#include "anki/common/robot/config.h"
#include "anki/common/robot/matlabInterface.h"

//#include <float.h>
//#pragma fenv_access (on)
//#pragma STDC FENV_ACCESS ON

#if ANKICORETECH_EMBEDDED_USE_GTEST
#include "gtest/gtest.h"
#endif

#if ANKICORETECH_EMBEDDED_USE_MATLAB
Anki::Embedded::Matlab matlab(false);
#endif

#if ANKICORETECH_EMBEDDED_USE_GTEST
int main(int argc, char ** argv)
#else
s32 RUN_ALL_COMMON_TESTS(s32 &numPassedTests, s32 &numFailedTests);
s32 RUN_ALL_VISION_TESTS(s32 &numPassedTests, s32 &numFailedTests);
int main()
#endif
{
  //// Set the floating point rounding to nearest

  //unsigned int control_word;
  //int err = _controlfp_s(&control_word, 0, 0);

  //AnkiConditionalErrorAndReturnValue(err == 0,
  //  -1, "main", "_controlfp_s failed");

  //{
  //  const s32 d1 = -1.1f;
  //  const s32 d2 = 1.1f;

  //  //printf("%d %d %d 0x%x\n", static_cast<s32>(1.4), static_cast<s32>(1.5), static_cast<s32>(1.6), control_word);
  //  printf("%d %d 0x%x\n", d1, d2, control_word);
  //}

  //err = _controlfp_s(&control_word, _RC_UP, _MCW_RC);

  //AnkiConditionalErrorAndReturnValue(err == 0,
  //  -1, "main", "_controlfp_s failed");

  //{
  //  const s32 d1 = -1.1f;
  //  const s32 d2 = 1.1f;

  //  //printf("%d %d %d 0x%x\n", static_cast<s32>(1.4), static_cast<s32>(1.5), static_cast<s32>(1.6), control_word);
  //  printf("%d %d 0x%x\n", d1, d2, control_word);
  //}

#if ANKICORETECH_EMBEDDED_USE_GTEST

  //char * filterString = "--gtest_filter=*"; // Run all tests
  char * filterString = "--gtest_filter=*RoundAndSaturate"; // Run only some tests

  argc++;
  argv[argc-1] = filterString;

  ::testing::InitGoogleTest(&argc, argv);

  const int result = RUN_ALL_TESTS();

#else // #if ANKICORETECH_EMBEDDED_USE_GTEST

  s32 numPassedTests_vision = 0;
  s32 numFailedTests_vision = 0;
  RUN_ALL_VISION_TESTS(numPassedTests_vision, numFailedTests_vision);

  s32 numPassedTests_common = 0;
  s32 numFailedTests_common = 0;
  RUN_ALL_COMMON_TESTS(numPassedTests_common, numFailedTests_common);

  const s32 numPassedTests = numPassedTests_common + numPassedTests_vision;
  const s32 numFailedTests = numFailedTests_common + numFailedTests_vision;

  printf(
    "\n"
    "========================================================================\n"
    "UNIT TEST RESULTS:\n"
    "Number Passed:%d\n"
    "Number Failed:%d\n"
    "========================================================================\n", numPassedTests, numFailedTests);

  const int result = numFailedTests;

#endif // #if ANKICORETECH_EMBEDDED_USE_GTEST ... #else

  printf("\n");
  return result;
}
#endif