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

#include "coretech/common/shared/types.h"
#include "anki/cozmo/shared/cozmoConfig.h"

namespace Anki {
  namespace Cozmo {
    namespace LiftController {
      
      Result Init();

      // Enable/Disable motor.
      // When disabled, lift goes limp. If autoReEnable, lift will re-enable
      // if it doesn't move for a certain period of time
      void Enable();
      void Disable(bool autoReEnable = false);
      
      // Moves lift all the way down and sets that position to 0
      // Automatically enables motor if it was disabled
      void StartCalibrationRoutine(bool autoStarted = false);
      
      // Returns true if calibration has completed
      bool IsCalibrated();      
      
      bool IsCalibrating();
      
      // Puts motor in uncalibrated state
      void ClearCalibration();
      
      f32 GetAngularVelocity();
      
      // TODO: Get rid of SetAngularVelocty?
      void SetAngularVelocity(const f32 speed_rad_per_sec, const f32 accel_rad_per_sec2 = MAX_LIFT_ACCEL_RAD_PER_S2);
      
      // Command the desired height of the lift
      // If useVPG, the commanded position profile considers current velocity and honors max velocity/acceleration.
      // If not useVPG, desired height_mm is commanded instantaneously.
      void SetDesiredHeight(f32 height_mm,
                            f32 speed_rad_per_sec = MAX_LIFT_SPEED_RAD_PER_S,
                            f32 accel_rad_per_sec2 = MAX_LIFT_ACCEL_RAD_PER_S2,
                            bool useVPG = true);
      
      // Command the desired height of the lift
      // duration_seconds:  The time it should take for it to reach the desired height
      // acc_start_frac:    The fraction of duration that it should be accelerating at the start
      // acc_end_frac:      The fraction of duration that it should be slowing down to a stop at the end
      void SetDesiredHeightByDuration(f32 height_mm, f32 acc_start_frac, f32 acc_end_frac, f32 duration_seconds);
      
      f32 GetDesiredHeight();
      bool IsInPosition();
      
      // Whether or not the lift is moving.
      // False if speed is 0 for more than LIFT_STOP_TIME.
      bool IsMoving();
      
      // Returns the last height that was commanded via SetDesiredHeight()
      f32 GetLastCommandedHeightMM();
      
      // Get current height of the lift
      f32 GetHeightMM();
      
      // Get current angle of the lift
      f32 GetAngleRad();
      
      Result Update();

      void SetGains(const f32 kp, const f32 ki, const f32 kd, const f32 maxIntegralError);

      // Momentarily enables motor even if it was disabled
      void Brace();
      void Unbrace();
      bool IsBracing();
      
      // Stops any nodding or movement at all.
      void Stop();
      
      void SendLiftLoadMessage(bool hasLoad);
      
      // Cuts power to lift briefly and checks for motion.
      // If there's motion it's assumed that the lift is carrying something.
      // Calls callback upon completion of check
      void CheckForLoad(void (*callback)(bool) = SendLiftLoadMessage);
      
    } // namespace LiftController
  } // namespace Cozmo
} // namespace Anki

#endif // COZMO_LIFT_CONTROLLER_H_
