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

#include "anki/common/types.h"

namespace Anki {
  namespace Cozmo {
    namespace TestModeController {
      
      typedef enum {
        TM_NONE,
        TM_PATH_FOLLOW,
        TM_DIRECT_DRIVE,
        TM_LIFT,
        TM_MAX_POWER_TEST
      } TestMode;
      
      
      // Sets up controller to run specified test mode
      ReturnCode Init(TestMode mode);

      ReturnCode Update();
      
    } // namespace TestModeController
  } // namespcae Cozmo
} // namespace Anki

#endif // TEST_MODE_CONTROLLER_H_