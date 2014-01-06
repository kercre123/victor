/**
File: run_myriad_embeddedCommonTests.cpp
Author: Peter Barnum
Created: 2013

Basic unit test calling utility

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/config.h"
#include "anki/common/robot/utilities.h"

int RUN_ALL_TESTS();

namespace Anki
{
  namespace Cozmo
  {
    namespace Robot
    {
      void Init()
      {
        printf("\n\n\nStarting unit tests\n\n");

        swcLeonFlushCaches();

        RUN_ALL_TESTS();

        printf("Finished unit tests\n\n\n\n");
        while(1)
          ;     // Because we don't trust hal.cpp
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