#include "liftController.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/common/robot/utilities_c.h"
#include "anki/common/shared/radians.h"

#include "cmath"

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
      
      void SetDesiredHeight(const f32 height_mm)
      {
        // Convert desired height into the necessary angle:
        desiredAngle_ = asinf((height_mm - LIFT_JOINT_HEIGHT)/LIFT_LENGTH);
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
          currentAngle_ = HAL::GetLiftAngle();
          angleError_ = desiredAngle_ - currentAngle_;
          
          // TODO: convert angleError_ to power / speed in some reasonable way
          if(ABS(angleError_) < ANGLE_TOLERANCE) {
            inPosition_ = true;
            HAL::SetLiftAngularVelocity(0.f);
          } else {
            inPosition_ = false;
            HAL::SetLiftAngularVelocity(Kp * angleError_.ToFloat());
          }
        } // if not in position
        
        return EXIT_SUCCESS;
      }
    } // namespace LiftController
  } // namespace Cozmo
} // namespace Anki