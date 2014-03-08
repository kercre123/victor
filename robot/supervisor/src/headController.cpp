#include "headController.h"
#include "anki/cozmo/robot/debug.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/common/robot/utilities_c.h"
#include "anki/common/shared/radians.h"
#include "anki/common/shared/velocityProfileGenerator.h"

namespace Anki {
  namespace Cozmo {
  namespace HeadController {

    namespace {
    
      const Radians ANGLE_TOLERANCE = DEG_TO_RAD(0.5f);
    
      // Minimum power required to move head
      const f32 MIN_POWER = 0.1;  // TODO: Measure what this actually is. approx is ok.
      f32 minPower_ = 0.f;
      
      // Currently applied power
      f32 power_ = 0;
      
      // Head angle control vars
      // 0 radians == looking straight ahead
      Radians currentAngle_ = 0.f;
      Radians desiredAngle_ = 0.f;
      f32 currDesiredAngle_ = 0.f;
      f32 currDesiredRadVel_ = 0.f;
      f32 angleError_ = 0.f;
      f32 angleErrorSum_ = 0.f;
      f32 prevHalPos_ = 0.f;
      bool inPosition_  = true;
      
      const f32 SPEED_FILTERING_COEFF = 0.9f;

      const f32 Kp_ = 0.5f; // proportional control constant
      const f32 Ki_ = 0.001f; // integral control constant
      const f32 MAX_ERROR_SUM = 200.f;
     
      // Current speed
      f32 radSpeed_ = 0.f;
      
      // Speed and acceleration params
      f32 maxSpeedRad_ = 1.0f;
      f32 accelRad_ = 2.0f;
      f32 approachSpeedRad_ = 0.1f;
      
      // For generating position and speed profile
      VelocityProfileGenerator vpg_;
      
      
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
      return currentAngle_.ToFloat();
    }
    
    void PoseAndSpeedFilterUpdate()
    {
      // Get encoder speed measurements
      f32 measuredSpeed = Cozmo::HAL::MotorGetSpeed(HAL::MOTOR_HEAD);
      
      radSpeed_ = (measuredSpeed *
                   (1.0f - SPEED_FILTERING_COEFF) +
                   (radSpeed_ * SPEED_FILTERING_COEFF));
      
      // Update position
      currentAngle_ += (HAL::MotorGetPosition(HAL::MOTOR_HEAD) - prevHalPos_);
      
#if(DEBUG_HEAD_CONTROLLER)
      PRINT("HEAD FILT: speed %f, speedFilt %f, currentAngle %f, currHalPos %f, prevPos %f, pwr %f\n",
            measuredSpeed, radSpeed_, currentAngle_.ToFloat(), HAL::MotorGetPosition(HAL::MOTOR_HEAD), prevHalPos_, power_);
#endif
      prevHalPos_ = HAL::MotorGetPosition(HAL::MOTOR_HEAD);
    }

    
    void SetAngularVelocity(const f32 rad_per_sec)
    {
      // TODO: Figure out power-to-speed ratio on actual robot. Normalize with battery power?
      f32 power = CLIP(rad_per_sec / HAL::MAX_HEAD_SPEED, -1.0, 1.0);
      HAL::MotorSetPower(HAL::MOTOR_HEAD, power);
      inPosition_ = true;
    }
    
    void SetSpeedAndAccel(f32 max_speed_rad_per_sec, f32 accel_rad_per_sec2)
    {
      maxSpeedRad_ = MAX(ABS(max_speed_rad_per_sec), approachSpeedRad_);
      accelRad_ = accel_rad_per_sec2;
    }
    
    void SetDesiredAngle(const f32 angle)
    {
#if(DEBUG_HEAD_CONTROLLER)
      PRINT("HEAD: SetDesiredAngle %f rads\n", desiredAngle_.ToFloat());
#endif
      
      desiredAngle_ = angle;
      angleError_ = desiredAngle_.ToFloat() - currentAngle_.ToFloat();
      angleErrorSum_ = 0.f;

      // Minimum power required to move the head
      minPower_ = MIN_POWER;
      if (angleError_ < 0) {
        minPower_ = -MIN_POWER;
      }

      
      inPosition_ = false;

      if (FLT_NEAR(angleError_,0.f)) {
        inPosition_ = true;
        PRINT("Head: Already at desired position\n");
        return;
      }
      
      // Start profile of head trajectory
      vpg_.StartProfile(radSpeed_, currentAngle_.ToFloat(),
                        maxSpeedRad_, accelRad_,
                        approachSpeedRad_, desiredAngle_.ToFloat(),
                        CONTROL_DT);
      
#if(DEBUG_HEAD_CONTROLLER)
      PRINT("HEAD VPG: startVel %f, startPos %f, maxVel %f, endVel %f, endPos %f\n",
            radSpeed_, currentAngle_.ToFloat(), maxSpeedRad_, approachSpeedRad_, desiredAngle_.ToFloat());
#endif

      
    }
    
    bool IsInPosition(void) {
      return inPosition_;
    }
    
    ReturnCode Update()
    {
      PoseAndSpeedFilterUpdate();
      
      if (!enable_)
        return EXIT_SUCCESS;
      
      
      // Note that a new call to SetDesiredAngle will get
      // Update() working again after it has reached a previous
      // setting.
      if(not inPosition_) {

        // Get the current desired lift angle
        vpg_.Step(currDesiredRadVel_, currDesiredAngle_);
        
#if(DEBUG_HEAD_CONTROLLER)
        PRINT("HEAD: currAngle %f, error %f\n", currentAngle_.ToFloat(), angleError_);
#endif
        
        // Compute current angle error
        angleError_ = currDesiredAngle_ - currentAngle_.ToFloat();
        
        
        // Convert angleError_ to power
        if(ABS(angleError_) < ANGLE_TOLERANCE) {
          angleErrorSum_ = 0.f;
          
          // If desired angle is low position, let it fall through to recalibration
          //if (!(RECALIBRATE_AT_LIMIT && desiredAngle_.ToFloat() == LIFT_ANGLE_LOW)) {
            power_ = 0.f;
            
            if (desiredAngle_ == currDesiredAngle_) {
              inPosition_ = true;
#if(DEBUG_HEAD_CONTROLLER)
              PRINT(" HEAD ANGLE REACHED (%f rad)\n", currentAngle_.ToFloat());
#endif
            }
          //}
        } else {
          power_ = minPower_ + (Kp_ * angleError_) + (Ki_ * angleErrorSum_);
          angleErrorSum_ += angleError_;
          angleErrorSum_ = CLIP(angleErrorSum_, -MAX_ERROR_SUM, MAX_ERROR_SUM);
          inPosition_ = false;
        }
        
#if(DEBUG_HEAD_CONTROLLER)
        PERIODIC_PRINT(100, "HEAD: currA %f, curDesA %f, desA %f, err %f, errSum %f, pwr %f, spd %f\n",
                       currentAngle_.ToFloat(),
                       currDesiredAngle_,
                       desiredAngle_.ToFloat(),
                       angleError_,
                       angleErrorSum_,
                       power_,
                       radSpeed_);
        PERIODIC_PRINT(100, "  POWER terms: %f  %f\n", (Kp_ * angleError_), (Ki_ * angleErrorSum_))
#endif
        
        power_ = CLIP(power_, -1.0, 1.0);
        
/*
        // If within 5 degrees of LIFT_HEIGHT_LOW and the lift isn't moving while downward power is applied,
        // assume we've hit the limit and recalibrate.
        if (limitingDetected_ ||
              ((power_ < 0)
               && (desiredAngle_.ToFloat() == LIFT_ANGLE_LOW)
               && (desiredAngle_.ToFloat() == currDesiredAngle_)
               && (ABS(angleError_) < 0.1)
               && NEAR_ZERO(HAL::MotorGetSpeed(HAL::MOTOR_LIFT)))) {
                
          if (!limitingDetected_) {
#if(DEBUG_LIFT_CONTROLLER)
            PRINT("START RECAL LIFT\n");
#endif
            lastLiftMovedTime_us = HAL::GetMicroCounter();
            limitingDetected_ = true;
          } else if (HAL::GetMicroCounter() - lastLiftMovedTime_us > 200000) {
#if(DEBUG_LIFT_CONTROLLER)
            PRINT("END RECAL LIFT\n");
#endif
            ResetLowAnglePosition();
            inPosition_ = true;
          }
          power_ = 0.f;
            
        }
*/
        
        HAL::MotorSetPower(HAL::MOTOR_HEAD, power_);

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
        
      } // if not in position
      
      return EXIT_SUCCESS;
    }
  } // namespace HeadController
  } // namespace Cozmo
} // namespace Anki
