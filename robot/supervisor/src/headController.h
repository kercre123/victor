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

#include "anki/types.h"

namespace Anki {
  namespace Cozmo {
    namespace HeadController {
      
      // TODO: Add if/when needed?
      // Result Init();

      void StartCalibrationRoutine();
      bool IsCalibrated();
      
      // Enable/Disable HAL commands to head motor
      void Enable();
      void Disable();
      
      // TODO: Do we want to keep SetAngularVelocity?
      void SetAngularVelocity(const f32 rad_per_sec);

      // Specifies max velocity and acceleration that SetDesiredAngle() uses.
      void SetMaxSpeedAndAccel(const f32 max_speed_rad_per_sec, const f32 accel_rad_per_sec2);
      void GetMaxSpeedAndAccel(f32 &max_speed_rad_per_sec, f32 &accel_rad_per_sec2);
      
      // Set the desired angle of head
      void SetDesiredAngle(f32 angle);
      
      // Set the desired angle of head
      // duration_seconds:  The time it should take for it to reach the desired angle
      // acc_start_frac:    The fraction of duration that it should be accelerating at the start
      // acc_end_frac:      The fraction of duration that it should be slowing down to a stop at the end
      void SetDesiredAngle(f32 angle, f32 acc_start_frac, f32 acc_end_frac, f32 duration_seconds);
      
      f32 GetAngularVelocity();
      
      bool IsInPosition();
      bool IsMoving();
      
      // Nod head between the two given angles at the given speed, until
      // SetDesiredAngle() or StopNodding() are called or the number of loops (up/down cycles)
      // is completed. If StopNodding() is called, head will be returned to the original
      // angle it started at. Use numLoops <= 0 to nod "forever".
      void StartNodding(const f32 lowAngle, const f32 highAngle, const u16 period_ms, const s32 numLoops,
                        const f32 easeInFraction, const f32 easeOutFraction);
      void StopNodding();
      bool IsNodding();
      
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
      
      // Stop nodding or any movement immediately
      void Stop();
      
    } // namespace HeadController
  } // namespcae Cozmo
} // namespace Anki

#endif // COZMO_HEAD_CONTROLLER_H_
