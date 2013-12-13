///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     main leon file 
///
extern "C" {
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "isaac_registers.h"
#include "mv_types.h"
#include "DrvSvu.h"
#include <DrvSvuDebug.h>
#include "DrvCpr.h"
#include "DrvTimer.h"
#include <DrvL2Cache.h>

#include "swcShaveLoader.h"
#include "swcTestUtils.h"

#include "cppInterface.h"
}

#include "anki/common/robot/utilities_c.h"
#include "anki/common/types.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace Robot
    {
      performanceStruct perfStr;

      void Init()
      {
        u32          i;
        u32          *fl;
        u32          test_pass = 1;

        printf("Starting unit tests \n");

        swcShaveProfInit(&perfStr);

        perfStr.countShCodeRun = 2;

        swcLeonFlushCaches();

        swcShaveProfStartGathering(0, &perfStr);

        runTests();

        swcShaveProfStopGathering(0, &perfStr);

        //    The below printf is modified from swcShaveProfPrint(0, &perfStr);
      //  explicitPrintf(1, "\nLeon executed %d cycles in %d micro seconds ([%d ms])\n",(s32)(perfStr.perfCounterTimer), (s32)(DrvTimerTicksToMs(perfStr.perfCounterTimer)*1000), (s32)(DrvTimerTicksToMs(perfStr.perfCounterTimer)));

        printf("Finished unit tests \n");
        while (1)
          ;       // Because we don't trust hal.cpp
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
