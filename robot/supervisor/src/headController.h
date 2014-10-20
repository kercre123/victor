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

#include "anki/common/types.h"

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
      void SetSpeedAndAccel(const f32 max_speed_rad_per_sec, const f32 accel_rad_per_sec2);
      void GetSpeedAndAccel(f32 &max_speed_rad_per_sec, f32 &accel_rad_per_sec2);
      
      // Set the desired angle of head
      void SetDesiredAngle(f32 angle);
      bool IsInPosition();
      bool IsMoving();
      
      // Nod head between the two given angles at the given speed, until
      // SetDesiredAngle() or StopNodding() are called or the number of loops (up/down cycles)
      // is completed. If StopNodding() is called, head will be returned to the original
      // angle it started at. Use numLoops <= 0 to nod "forever".
      void StartNodding(const f32 lowAngle, const f32 highAngle, const f32 speed, const f32 accel, const s32 numLoops);
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
      
      void SetGains(const f32 kp, const f32 ki, const f32 maxIntegralError);
      
    } // namespace HeadController
  } // namespcae Cozmo
} // namespace Anki

#endif // COZMO_HEAD_CONTROLLER_H_
