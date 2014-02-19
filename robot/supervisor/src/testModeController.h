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
        
        // Attempts to dock to a block that is placed in front of it and then place it on a block behind it.
        TM_PICK_AND_PLACE,
        
        // Follows a changing straight line path. Tests path following during docking.
        TM_DOCK_PATH,
        
        // Follows an example path. Requires localization
        TM_PATH_FOLLOW,
        
        // Tests ExecuteDirectDrive() or open loop control via HAL::MotorSetPower()
        TM_DIRECT_DRIVE,

        // Moves lift up and down
        TM_LIFT,
        
        // Toggles between 3 main lift heights: low dock, carry, and high dock
        TM_LIFT_TOGGLE,
        
        // Tilts head up and down
        TM_HEAD,
        
        // Engages and disengages gripper
        TM_GRIPPER,
        
        // Drives slow and then stops.
        // Drives fast and then stops.
        // Reports stopping distance and time (in tics).
        TM_STOP_TEST,
        
        // Drives all motors at max power simultaneously.
        TM_MAX_POWER_TEST
      } TestMode;
      
      
      // Sets up controller to run specified test mode
      ReturnCode Init(TestMode mode);

      ReturnCode Update();
      
      TestMode GetMode();
      
    } // namespace TestModeController
  } // namespcae Cozmo
} // namespace Anki

#endif // TEST_MODE_CONTROLLER_H_
