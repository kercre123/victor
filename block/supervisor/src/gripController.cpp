#include "gripController.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"

#if defined(HAVE_ACTIVE_GRIPPER) && HAVE_ACTIVE_GRIPPER

namespace Anki {
  namespace Cozmo {
    namespace GripController {
      
      namespace {
        
        // Constants
        
        // Power that is applied when engaging the gripper
        const f32 GRIPPER_ENGAGE_POWER = -0.5;
        
        // Power that is applied when disengaging the gripper
        const f32 GRIPPER_DISENGAGE_POWER = 0.5;
        
        // How long power should be applied when engaging/disengaging the gripper
        const u32 GRIPPER_MOTOR_ENGAGE_TIME_US = 600000;
        
        // The time when power was applied to the gripper
        u32 gripperPowerAppliedTime_ = 0;
        
        // Current power applied to gripper
        f32 gripperPower_ = false;
        
        // Whether or not gripper is engaged
        // TODO: This should be handled by HAL::IsGripperEngaged() once that's working.
        bool gripperEngaged_ = false;
        
      } // "private" namespace
      
      void PowerGripper(f32 power)
      {
        if (power != gripperPower_) {
          gripperPower_ = power;
          //HAL::MotorSetPower(HAL::MOTOR_GRIP, gripperPower_);
          gripperPowerAppliedTime_ = HAL::GetMicroCounter();
          //PRINT(" grip power %f\n", gripperPower_);
        }
      }
      
      void EngageGripper()
      {
        PowerGripper(GRIPPER_ENGAGE_POWER);
      }
      
      void DisengageGripper()
      {
        PowerGripper(GRIPPER_DISENGAGE_POWER);
        gripperEngaged_ = false;
      }
      
      bool IsGripperEngaged()
      {
#ifdef SIMULATOR
        return HAL::IsGripperEngaged();
#else
        // TODO: Since there is currently no sensor for the gripper,
        //       just assume for now that gripper is engaged after
        //       positive power has been applied.
        return gripperEngaged_;
#endif
      }
      
      Result Update()
      {
        if (HAL::GetMicroCounter() - gripperPowerAppliedTime_ > GRIPPER_MOTOR_ENGAGE_TIME_US) {
          if (gripperPower_ > 0) {
            gripperEngaged_ = true;
          }
          PowerGripper(0.f);
        }
        return RESULT_OK;
      }
      

    } // namespace GripController
  } // namespace Cozmo
} // namespace Anki

#endif
