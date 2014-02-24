/**
 * File: liftController.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 10/28/2013
 *
 * Description:
 *   Controller for adjusting the lifter height.
 *
 * Copyright: Anki, Inc. 2013
 *
 **/

#ifndef COZMO_LIFT_CONTROLLER_H_
#define COZMO_LIFT_CONTROLLER_H_

#include "anki/common/types.h"

namespace Anki {
  namespace Cozmo {
    namespace LiftController {
      
      ReturnCode Init();

      // Enable/Disable commands to the motor via HAL functions
      // Mostly for debug.
      void Enable();
      void Disable();
      
      // Moves lift all the way down and sets that position to 0
      void StartCalibrationRoutine();
      
      // Returns true if calibration has completed
      bool IsCalibrated();

      // Specifies max velocity and acceleration that SetDesiredHeight() uses.
      void SetSpeedAndAccel(f32 max_speed_rad_per_sec, f32 accel_rad_per_sec2);
      
      f32 GetAngularVelocity();
      
      // TODO: Get rid of SetAngularVelocty?
      void SetAngularVelocity(const f32 rad_per_sec);
      
      // Command the desired height of the lift
      void SetDesiredHeight(const f32 height_mm);
      bool IsInPosition();
      
      // Get current height of the lift
      f32 GetHeightMM();
      
      // Get current angle of the lift
      f32 GetAngleRad();
      
      ReturnCode Update();
      
    } // namespace LiftController
  } // namespcae Cozmo
} // namespace Anki

#endif // COZMO_LIFT_CONTROLLER_H_
