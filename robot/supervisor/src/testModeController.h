/**
 * File: testModeController.h
 *
 * Author: Kevin Yoon (kevin)
 * Created: 11/13/2013
 *
 *
 * Description:
 *
 *   Controller for executing various test modes (for development).
 *
 * Copyright: Anki, Inc. 2013
 *
 **/


#ifndef TEST_MODE_CONTROLLER_H_
#define TEST_MODE_CONTROLLER_H_

#include "coretech/common/shared/types.h"
#include "clad/types/robotTestModes.h"

namespace Anki {
  namespace Vector {
    namespace TestModeController {
      
      // Sets up controller to run specified test mode
      Result Start(const TestMode mode, s32 p1 = 0, s32 p2 = 0, s32 p3 = 0);

      Result Update();
      
      TestMode GetMode();
      
    } // namespace TestModeController
  } // namespace Vector
} // namespace Anki

#endif // TEST_MODE_CONTROLLER_H_
