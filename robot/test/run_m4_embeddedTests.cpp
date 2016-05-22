/**
File: run_m4_embeddedTests.cpp
Author: Peter Barnum
Created: 2014-02-14

Basic unit test calling utility for common and vision tests

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/config.h"
#include "anki/common/robot/utilities.h"

s32 RUN_ALL_COMMON_TESTS(s32 &numPassedTests, s32 &numFailedTests);
s32 RUN_ALL_VISION_TESTS(s32 &numPassedTests, s32 &numFailedTests);

namespace Anki
{
  namespace Cozmo
  {
    namespace Robot
    {
      void Init()
      {
        printf("\n\n\nStarting unit tests\n\n");

        s32 numPassedTests_common = 0;
        s32 numFailedTests_common = 0;
        RUN_ALL_COMMON_TESTS(numPassedTests_common, numFailedTests_common);
        
				s32 numPassedTests_vision = 0;
        s32 numFailedTests_vision = 0;
        RUN_ALL_VISION_TESTS(numPassedTests_vision, numFailedTests_vision);
				
        const s32 numPassedTests = numPassedTests_common + numPassedTests_vision;
        const s32 numFailedTests = numFailedTests_common + numFailedTests_vision;
        
        printf(
          "\n"
          "========================================================================\n"
          "UNIT TEST RESULTS:\n"
          "Number Passed:%d\n"
          "Number Failed:%d\n"
          "========================================================================\n", numPassedTests, numFailedTests);
  
#ifndef USING_SIMULATOR
        while(1)
          ;     // Because we don't trust hal.cpp
#endif
      }

      void step_MainExecution()
      {
      }

      void step_LongExecution()
      {
      }
    }
  }
}
