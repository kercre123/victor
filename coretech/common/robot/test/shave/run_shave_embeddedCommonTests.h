#ifndef _RUN_SHAVE_EMBEDDED_COMMON_TESTS_H_
#define _RUN_SHAVE_EMBEDDED_COMMON_TESTS_H_

#include "anki/common/robot/config.h"

#define MAX_SHAVE_BYTES 50000

extern volatile s32 shave0_whichTest;
extern char shave0_buffer[MAX_SHAVE_BYTES];

#ifdef __cplusplus
extern "C" {
#endif

  // Test 0: GTEST_TEST(CoreTech_Common, ShavePrintfTest)
  extern int shave0_main();

  // Test 1: GTEST_TEST(CoreTech_Common, ShaveAddTest)
  extern const s32 * restrict shave0_pIn1;
  extern const s32 * restrict shave0_pIn2;
  extern s32 * restrict shave0_pOut;
  extern s32 shave0_numElements;

#ifdef __cplusplus
}
#endif

#endif // #ifndef _RUN_SHAVE_EMBEDDED_COMMON_TESTS_H_