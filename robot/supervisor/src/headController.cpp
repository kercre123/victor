#include "headController.h"
#include "anki/cozmo/robot/debug.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/common/robot/utilities_c.h"
#include "anki/common/shared/radians.h"

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
     
      // TODO: Unused, remove?
      //f32 radSpeed_ = 0.f;
      
      bool enable_ = true;
      
    } // "private" members

    
    void Enable()
    {
      enable_ = true;
    }
    
    void Disable()
    {
      enable_ = false;
    }

    f32 GetAngleRad()
    {
      return HAL::MotorGetPosition(HAL::MOTOR_HEAD);
    }
    
    void SetAngularVelocity(const f32 rad_per_sec)
    {
      // TODO: Figure out power-to-speed ratio on actual robot. Normalize with battery power?
      f32 power = CLIP(rad_per_sec / HAL::MAX_HEAD_SPEED, -1.0, 1.0);
      HAL::MotorSetPower(HAL::MOTOR_HEAD, power);
      inPosition_ = true;
    }
    
    
    void SetDesiredAngle(const f32 angle)
    {
      desiredAngle_ = angle;
      inPosition_ = false;
#if(DEBUG_HEAD_CONTROLLER)
      PRINT("HEAD: SetDesiredAngle %f rads\n", desiredAngle_.ToFloat());
#endif
    }
    
    bool IsInPosition(void) {
      return inPosition_;
    }
    
    ReturnCode Update()
    {
      if (!enable_)
        return EXIT_SUCCESS;
      
      
      // Note that a new call to SetDesiredAngle will get
      // Update() working again after it has reached a previous
      // setting.
      if(not inPosition_) {
        
        // Simple proportional control for now
        // TODO: better controller?
        currentAngle_ = HAL::MotorGetPosition(HAL::MOTOR_HEAD);
        angleError_ = desiredAngle_ - currentAngle_;

#if(DEBUG_HEAD_CONTROLLER)
        PRINT("HEAD: currAngle %f, error %f\n", currentAngle_.ToFloat(), angleError_.ToFloat());
#endif
        
        // TODO: convert angleError_ to power / speed in some reasonable way
        if(ABS(angleError_) < ANGLE_TOLERANCE) {
          SetAngularVelocity(0.f);
          inPosition_ = true;
          
#if USE_OFFBOARD_VISION
          // Keep offboard vision processor apprised of the current head angle for
          // computing the docking error signal
          {
            Messages::RobotState stateMsg;
            stateMsg.timestamp = HAL::GetTimeStamp();
            stateMsg.headAngle = currentAngle_.ToFloat();
            HAL::USBSendPacket(HAL::USB_VISION_COMMAND_ROBOTSTATE,
                               &stateMsg, sizeof(Messages::RobotState));
          }
#endif
          
        } else {
          SetAngularVelocity(Kp * angleError_.ToFloat());
          inPosition_ = false;
        }
        
      } // if not in position
      
      return EXIT_SUCCESS;
    }
  } // namespace HeadController
  } // namespace Cozmo
} // namespace Anki