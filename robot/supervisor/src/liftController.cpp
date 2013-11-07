#include "liftController.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/vehicleMath.h"
#include "anki/common/robot/utilities_c.h"
#include "anki/common/shared/radians.h"


namespace Anki {
  namespace Cozmo {
    namespace LiftController {
      
      namespace {
        // All angles will be measured in DEGREES.
        
        const f32 Kp = 5.f; // proportional control constant
        const Radians ANGLE_TOLERANCE = DEG_TO_RAD(0.5f);
        
        Radians currentAngle_ = 0.f;
        Radians desiredAngle_ = 0.f;
        Radians angleError_   = 1e9f;
        bool inPosition_  = true;
        
      } // "private" members
      
      void SetAngularVelocity(const f32 rad_per_sec)
      {
        // TODO: Figure out power-to-speed ratio on actual robot. Normalize with battery power?
        f32 power = CLIP(rad_per_sec / HAL::MAX_LIFT_SPEED, -1.0, 1.0);
        HAL::MotorSetPower(HAL::MOTOR_LIFT, power);
        inPosition_ = true;
      }
      
      
      void SetDesiredHeight(const f32 height_mm)
      {
        // Convert desired height into the necessary angle:
        desiredAngle_ = asin_fast((height_mm - LIFT_JOINT_HEIGHT)/LIFT_LENGTH);
        inPosition_ = false;
      }
      
      bool IsInPosition(void) {
        return inPosition_;
      }
      
      ReturnCode Update()
      {
        // Note that a new call to SetDesiredAngle will get
        // Update() working again after it has reached a previous
        // setting.
        if(not inPosition_) {
          
          // Simple proportional control for now
          // TODO: better controller?
          currentAngle_ = HAL::MotorGetPosition(HAL::MOTOR_LIFT);
          angleError_ = desiredAngle_ - currentAngle_;
          
          // TODO: convert angleError_ to power / speed in some reasonable way
          if(ABS(angleError_) < ANGLE_TOLERANCE) {
            inPosition_ = true;
            SetAngularVelocity(0.f);
          } else {
            inPosition_ = false;
            SetAngularVelocity(Kp * angleError_.ToFloat());
          }
        } // if not in position
        
        return EXIT_SUCCESS;
      }
    } // namespace LiftController
  } // namespace Cozmo
} // namespace Anki