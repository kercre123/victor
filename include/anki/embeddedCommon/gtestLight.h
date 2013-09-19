#ifndef _ANKICORETECHEMBEDDED_COMMON_GTEST_LIGHT_H_
#define _ANKICORETECHEMBEDDED_COMMON_GTEST_LIGHT_H_

#include "anki/embeddedCommon/config.h"

#ifdef ANKICORETECHEMBEDDED_USE_GTEST

// To prevent a warning (or error) about a function not returning a value, include this macro at the end of any GTEST_TEST
#define GTEST_RETURN_HERE

#else

// To prevent a warning (or error) about a function not returning a value, include this macro at the end of any GTEST_TEST
#define GTEST_RETURN_HERE {printf("Test completed\n"); return 0; }

// Same usage as the Gtest macro
#define GTEST_TEST(test_case_name, test_name) s32 test_case_name ## test_name()

// Same usage as the Gtest macro
#define ASSERT_TRUE(condition)\
  if(!(condition)) { \
  _Anki_Logf(\
  "\n------------------------------------------------------------------------\nUnitTestAssert(" #condition ") is false\nUnit Test Assert Failure\n------------------------------------------------------------------------",\
  "", __FILE__, __PRETTY_FUNCTION__, __LINE__); \
  return -1;\
  }

// Same usage as the Gtest macro
#define ASSERT_FALSE(condition) ASSERT_TRUE(!(condition))

// Call a GTEST_TEST, and increment the variable numPassedTests if the test passed, and  the variable numFailedTests if the test failed.
#define CALL_GTEST_TEST(test_case_name, test_name)\
  if(test_case_name ## test_name() == 0) {\
  printf("\n"); printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\nPASSED:" #test_case_name # test_name "\n");\
  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n"); numPassedTests++;\
  } else {\
  printf("\n");\
  printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\nFAILED:" #test_case_name # test_name "\n"); \
  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n"); numFailedTests++; };

#endif // ANKICORETECHEMBEDDED_USE_GTEST

#endif // #ifndef _ANKICORETECHEMBEDDED_COMMON_GTEST_LIGHT_H_
