/**
File: gtestLight.h
Author: Peter Barnum
Created: 2013

This file contains API-compatible versions of the Google Test framework, for use on an embedded system.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_GTEST_LIGHT_H_
#define _ANKICORETECHEMBEDDED_COMMON_GTEST_LIGHT_H_

#include "coretech/common/robot/config.h"

#if ANKICORETECH_EMBEDDED_USE_GTEST

// To prevent a warning (or error) about a function not returning a value, include this macro at the end of any GTEST_TEST. Also, this lets you set a breakpoint at the end.
#define GTEST_RETURN_HERE {CoreTechPrint(""); return;}

#else // #if ANKICORETECH_EMBEDDED_USE_GTEST

// To prevent a warning (or error) about a function not returning a value, include this macro at the end of any GTEST_TEST
#define GTEST_RETURN_HERE {CoreTechPrint(""); return 0; }

// Same usage as the Gtest macro
#define GTEST_TEST(test_case_name, test_name) s32 test_case_name ## __ ## test_name()

// Same usage as the Gtest macro
#define ASSERT_TRUE(condition)\
  if(!(condition)) { \
  _Anki_Log(ANKI_LOG_LEVEL_ERROR,\
  "\n----------------------------------------------------------------------\nUnitTestAssert(" #condition ") is false\nUnit Test Assert Failure\n---------------------------------------------------------------------",\
  "", __FILE__, __PRETTY_FUNCTION__, __LINE__); \
  return -1;\
  }

// Same usage as the Gtest macro
#define ASSERT_FALSE(condition) ASSERT_TRUE(!(condition))

// Same usage as the Gtest macro
#define ASSERT_EQ(term1, term2)\
  ASSERT_TRUE((term1) == (term2))

#define ASSERT_LT(term1, term2)\
  ASSERT_TRUE((term1) < (term2))

#define ASSERT_LE(term1, term2)\
  ASSERT_TRUE((term1) <= (term2))

#define ASSERT_GT(term1, term2)\
  ASSERT_TRUE((term1) > (term2))

#define ASSERT_GE(term1, term2)\
  ASSERT_TRUE((term1) >= (term2))

#define ASSERT_NEAR(term1, term2, maxAbsoluteDifference)\
  ASSERT_TRUE( (term1 >= term2) ? (term1-term2) <= maxAbsoluteDifference : (term2-term1) <= maxAbsoluteDifference) // Handles unsigned comparisons

// Call a GTEST_TEST, and increment the variable numPassedTests if the test passed, and  the variable numFailedTests if the test failed.
#define CALL_GTEST_TEST(test_case_name, test_name)\
  if(test_case_name ## __ ## test_name() == 0) {\
  CoreTechPrint("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\nPASSED:" # test_case_name "__" # test_name "\n" "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");\
  numPassedTests++;\
  } else {\
  CoreTechPrint("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\nFAILED:" # test_case_name "__" # test_name "\n" "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n\n"); numFailedTests++; };

#endif // #if ANKICORETECH_EMBEDDED_USE_GTEST ... #else

#endif // #ifndef _ANKICORETECHEMBEDDED_COMMON_GTEST_LIGHT_H_
