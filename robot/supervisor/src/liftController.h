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
      
      Result Init();

      // Enable/Disable commands to the motor via HAL functions
      // Mostly for debug.
      void Enable();
      void Disable();
      
      // Moves lift all the way down and sets that position to 0
      void StartCalibrationRoutine();
      
      // Returns true if calibration has completed
      bool IsCalibrated();

      // Specifies max velocity and acceleration that SetDesiredHeight() uses.
      void SetSpeedAndAccel(const f32 max_speed_rad_per_sec, const f32 accel_rad_per_sec2);
      void GetSpeedAndAccel(f32 &max_speed_rad_per_sec, f32 &accel_rad_per_sec2);
      
      f32 GetAngularVelocity();
      
      // TODO: Get rid of SetAngularVelocty?
      void SetAngularVelocity(const f32 rad_per_sec);
      
      // Command the desired height of the lift
      void SetDesiredHeight(f32 height_mm);
      
      // Command the desired height of the lift
      // duration_seconds:  The time it should take for it to reach the desired height
      // acc_start_frac:    The fraction of duration that it should be accelerating at the start
      // acc_end_frac:      The fraction of duration that it should be slowing down to a stop at the end
      void SetDesiredHeight(f32 height_mm, f32 acc_start_frac, f32 acc_end_frac, f32 duration_seconds);
      
      f32 GetDesiredHeight();
      bool IsInPosition();

      // Whether or not the lift is moving.
      // False if speed is 0 for more than LIFT_STOP_TIME.
      bool IsMoving();
      
      // Whether or not the lift motor is limiting at the low or high
      // end of its range. If limitAngle is non-null, it will be made to
      // point to the angle at which it is limiting.
      bool IsLimiting(f32* limitAngle = 0);
      
      // True if the lift has not had power applied to it for more than LIFT_RELAX_TIME
      bool IsRelaxed();
      
      // Returns the last height that was commanded via SetDesiredHeight()
      f32 GetLastCommandedHeightMM();
      
      // Get current height of the lift
      f32 GetHeightMM();
      
      // Get current angle of the lift
      f32 GetAngleRad();
      
      Result Update();

      void SetGains(const f32 kp, const f32 ki, const f32 maxIntegralError);
      
    } // namespace LiftController
  } // namespcae Cozmo
} // namespace Anki

#endif // COZMO_LIFT_CONTROLLER_H_
