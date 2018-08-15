/**
 * File: headController.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 10/28/2013
 *
 * Information on last revision to this file:
 *    $LastChangedDate$
 *    $LastChangedBy$
 *    $LastChangedRevision$
 *
 * Description:
 *
 *   Controller for adjusting the head angle (and other aspects of the head
 *   as well, like LEDs?)
 *
 * Copyright: Anki, Inc. 2013
 *
 **/


#ifndef COZMO_HEAD_CONTROLLER_H_
#define COZMO_HEAD_CONTROLLER_H_

#include "coretech/common/shared/types.h"
#include "anki/cozmo/shared/cozmoConfig.h"

namespace Anki {
  namespace Vector {
    namespace HeadController {
      
      // TODO: Add if/when needed?
      // Result Init();

      void StartCalibrationRoutine(bool autoStarted = false);
      bool IsCalibrated();
      bool IsCalibrating();
      
      // Puts motor in uncalibrated state
      void ClearCalibration();
      
      // Enable/Disable HAL commands to head motor
      void Enable();
      void Disable(bool autoReEnable = false);
      
      // TODO: Do we want to keep SetAngularVelocity?
      void SetAngularVelocity(const f32 speed_rad_per_sec, const f32 accel_rad_per_sec2 = MAX_HEAD_ACCEL_RAD_PER_S2);
      
      // Set the desired angle of head
      // If useVPG, the commanded position profile considers current velocity and honors max velocity/acceleration.
      // If not useVPG, desired angle is commanded instantaneously.
      void SetDesiredAngle(f32 angle_rad,
                           f32 speed_rad_per_sec = MAX_HEAD_SPEED_RAD_PER_S,
                           f32 accel_rad_per_sec2 = MAX_HEAD_ACCEL_RAD_PER_S2,
                           bool useVPG = true);
      
      // Set the desired angle of head
      // duration_seconds:  The time it should take for it to reach the desired angle
      // acc_start_frac:    The fraction of duration that it should be accelerating at the start
      // acc_end_frac:      The fraction of duration that it should be slowing down to a stop at the end
      void SetDesiredAngleByDuration(f32 angle_rad, f32 acc_start_frac, f32 acc_end_frac, f32 duration_seconds);
      
      f32 GetAngularVelocity();
      
      bool IsInPosition();
      bool IsMoving();
      
      // Get current head angle
      f32 GetAngleRad();
      
      // Set current head angle
      void SetAngleRad(f32 angle);

      f32 GetLastCommandedAngle();
      
      // Returns the camera's pose with respect to the robot origin.
      // y is always 0 since head doesn't move laterally.
      void GetCamPose(f32 &x, f32 &z, f32 &angle);
      
      Result Update();
      
      void SetGains(const f32 kp, const f32 ki, const f32 kd, const f32 maxIntegralError);
      
      void Brace();
      void Unbrace();
      bool IsBracing();
      
      // Stop nodding or any movement immediately
      void Stop();
      
    } // namespace HeadController
  } // namespace Vector
} // namespace Anki

#endif // COZMO_HEAD_CONTROLLER_H_
