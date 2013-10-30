#include "headController.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/common/robot/utilities_c.h"

// TODO: this needs to get moved out of basestation
#include "anki/common/basestation/math/radians.h"

namespace Anki {
  namespace Cozmo {
  namespace HeadController {

    namespace {
      // All angles will be measured in DEGREES.
      
      const f32 Kp = 5.f; // proportional control constant
      const Radians ANGLE_TOLERANCE = DEG_TO_RAD(0.5f);
    
      Radians currentAngle_ = 0.f;
      Radians desiredAngle_ = 0.f;
      Radians angleError_   = 1e9f;
      bool inPosition_  = true;
      
    } // "private" members

    void SetDesiredAngle(const f32 angle)
    {
      desiredAngle_ = angle;
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
        currentAngle_ = HAL::GetHeadAngle();
        angleError_ = desiredAngle_ - currentAngle_;
        
        // TODO: convert angleError_ to power / speed in some reasonable way
        if(ABS(angleError_) < ANGLE_TOLERANCE) {
          inPosition_ = true;
          HAL::SetHeadAngularVelocity(0.f);
        } else {
          inPosition_ = false;
          HAL::SetHeadAngularVelocity(Kp * angleError_.ToFloat());
        }
        
      } // if not in position
      
      return EXIT_SUCCESS;
    }
  } // namespace HeadController
  } // namespace Cozmo
} // namespace Anki