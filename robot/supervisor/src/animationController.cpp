#include "animationController.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/common/robot/utilities_c.h"
#include "anki/common/shared/radians.h"
#include "anki/common/shared/velocityProfileGenerator.h"

#include "headController.h"
#include "liftController.h"
#include "wheelController.h"
#include "steeringController.h"
#include "speedController.h"


#define DEBUG_ANIMATION_CONTROLLER 0

namespace Anki {
namespace Cozmo {
namespace AnimationController {

  namespace {
    AnimationID_t currAnim_ = ANIM_IDLE;
    
    f32 headAngle_ = 0;
    const f32 HEAD_NOD_UPPER_ANGLE = DEG_TO_RAD(20);
    const f32 HEAD_NOD_LOWER_ANGLE = DEG_TO_RAD(-5);
    const f32 HEAD_NOD_SPEED_RAD_PER_S = 10;
    const f32 HEAD_NOD_ACCEL_RAD_PER_S2 = 40;
    
  } // "private" members

  
  void Update()
  {
    switch(currAnim_) {
      case ANIM_IDLE:
        break;
        
      case ANIM_HEAD_NOD:
      {
        if (HeadController::IsInPosition()) {
          if (HeadController::GetLastCommandedAngle() == HEAD_NOD_UPPER_ANGLE) {
            HeadController::SetDesiredAngle(HEAD_NOD_LOWER_ANGLE);
          } else if (HeadController::GetLastCommandedAngle() == HEAD_NOD_LOWER_ANGLE) {
            HeadController::SetDesiredAngle(HEAD_NOD_UPPER_ANGLE);
          }
        }
        break;
      }
    }
    
  }

  void PlayAnimation(AnimationID_t anim)
  {
    currAnim_ = anim;
    
    // Initialize the animation
    switch(currAnim_) {
      case ANIM_IDLE:
      {
        // Stop all motors, lights, and sounds
        LiftController::Enable();
        LiftController::SetAngularVelocity(0);
        HeadController::Enable();
        HeadController::SetAngularVelocity(0);

        SpeedController::SetBothDesiredAndCurrentUserSpeed(0);
        break;
      }
      case ANIM_HEAD_NOD:
      {
        HeadController::SetSpeedAndAccel(HEAD_NOD_SPEED_RAD_PER_S, HEAD_NOD_ACCEL_RAD_PER_S2);
        HeadController::SetDesiredAngle(HEAD_NOD_UPPER_ANGLE);
        break;
      }
    }
  }
  
} // namespace AnimationController
} // namespace Cozmo
} // namespace Anki
