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
      void SetSpeedAndAccel(f32 max_speed_rad_per_sec, f32 accel_rad_per_sec2);
      
      // Set the desired angle of head
      void SetDesiredAngle(f32 angle);
      bool IsInPosition();
      bool IsMoving();
      
      // Get current head angle
      f32 GetAngleRad();

      f32 GetLastCommandedAngle();
      
      Result Update();
      
    } // namespace HeadController
  } // namespcae Cozmo
} // namespace Anki

#endif // COZMO_HEAD_CONTROLLER_H_
