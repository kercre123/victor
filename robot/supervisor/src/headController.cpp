#include "headController.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/common/robot/utilities_c.h"
#include "anki/common/shared/radians.h"
#include "anki/common/shared/velocityProfileGenerator.h"
#include "anki/common/robot/errorHandling.h"

#define DEBUG_HEAD_CONTROLLER 0

namespace Anki {
namespace Cozmo {
namespace HeadController {

    namespace {
      
      // TODO: Ideally, this value should be calibrated
#ifdef SIMULATOR
      const Radians HEAD_CAL_OFFSET = DEG_TO_RAD(0);
#elif defined(COZMO_ROBOT_V40)
      const Radians HEAD_CAL_OFFSET = DEG_TO_RAD(-4);
#else
      const Radians HEAD_CAL_OFFSET = DEG_TO_RAD(2);
#endif
      
      const Radians ANGLE_TOLERANCE = DEG_TO_RAD(2.f);
      
      // Used when calling SetDesiredAngle with just an angle:
      const f32 DEFAULT_START_ACCEL_FRAC = 0.1f;
      const f32 DEFAULT_END_ACCEL_FRAC   = 0.1f;
      
      // Currently applied power
      f32 power_ = 0;
      
      // Head angle control vars
      // 0 radians == looking straight ahead
      Radians currentAngle_ = 0.f;
      Radians desiredAngle_ = 0.f;
      f32 currDesiredAngle_ = 0.f;
      f32 angleError_ = 0.f;
      f32 angleErrorSum_ = 0.f;
      f32 prevAngleError_ = 0.f;
      f32 prevHalPos_ = 0.f;
      bool inPosition_  = true;
      
      const f32 SPEED_FILTERING_COEFF = 0.5f;

#ifdef SIMULATOR
      f32 Kp_ = 20.f; // proportional control constant
      f32 Kd_ = 0.f;  // derivative control constant
      f32 Ki_ = 0.1f; // integral control constant
      f32 MAX_ERROR_SUM = 2.f;

      const f32 BASE_POWER  = 0.f;
#else
      f32 Kp_ = 4.f;  // proportional control constant
      f32 Kd_ = 4000.f;  // derivative control constant
      f32 Ki_ = 0.02f; // integral control constant
      f32 MAX_ERROR_SUM = 10.f;
      
      const f32 BASE_POWER  = 0.2f;
#endif
      
      // Current speed
      f32 radSpeed_ = 0.f;
      
      // Speed and acceleration params
      f32 maxSpeedRad_ = 1.0f;
      f32 accelRad_ = 2.0f;
      
      // For generating position and speed profile
      VelocityProfileGenerator vpg_;
      
      
      // Nodding
      bool isNodding_ = false;
      //f32 preNodAngle_  = 0.f;
      f32 nodLowAngle_  = 0.f;
      f32 nodHighAngle_ = 0.f;
      s32 numNodsDesired_ = 0;
      s32 numNodsComplete_ = 0;
      f32 nodHalfPeriod_sec_ = 0.5f;
      f32 nodEaseOutFraction_ = 0.5f;
      f32 nodEaseInFraction_  = 0.5f;
      
      // Calibration parameters
      typedef enum {
        HCS_IDLE,
        HCS_LOWER_HEAD,
        HCS_WAIT_FOR_STOP,
        HCS_SET_CURR_ANGLE
      } HeadCalibState;
      
      HeadCalibState calState_ = HCS_IDLE;
      bool isCalibrated_ = false;
      u32 lastHeadMovedTime_ms = 0;
      
      u32 lastInPositionTime_ms_;
      const u32 IN_POSITION_TIME_MS = 200;
      
      const f32 MAX_HEAD_CONSIDERED_STOPPED_RAD_PER_SEC = 0.001;
      
      const u32 HEAD_STOP_TIME = 500;  // ms
      
      bool enable_ = true;
      
    } // "private" members

    
    void Enable()
    {
      enable_ = true;
    }
    
    void Disable()
    {
      if(enable_) {
        enable_ = false;
        
        inPosition_ = true;
        angleErrorSum_ = 0.f;
        
        power_ = 0;
        HAL::MotorSetPower(HAL::MOTOR_HEAD, power_);
      }
    }
    

    void StartCalibrationRoutine()
    {
      PRINT("Starting Head calibration\n");
      
#ifdef SIMULATOR
      // Skipping actual calibration routine in sim due to weird lift behavior when attempting to move it when
      // it's at the joint limit.  The arm flies off the robot!
      isCalibrated_ = true;
      SetDesiredAngle(MIN_HEAD_ANGLE);
#else
      calState_ = HCS_LOWER_HEAD;
      isCalibrated_ = false;
#endif
    }
    
    bool IsCalibrated()
    {
      return isCalibrated_;
    }
    
    
    void ResetLowAnglePosition()
    {
      currentAngle_ = MIN_HEAD_ANGLE + HEAD_CAL_OFFSET;
      HAL::MotorResetPosition(HAL::MOTOR_HEAD);
      prevHalPos_ = HAL::MotorGetPosition(HAL::MOTOR_HEAD);
      isCalibrated_ = true;
    }
    
    bool IsMoving()
    {
      return (ABS(radSpeed_) > MAX_HEAD_CONSIDERED_STOPPED_RAD_PER_SEC);
    }
    
    void Stop()
    {
      isNodding_ = false;
      SetAngularVelocity(0);
    }
    
    void CalibrationUpdate()
    {
      if (!isCalibrated_) {
        
        switch(calState_) {
            
          case HCS_IDLE:
            break;
            
          case HCS_LOWER_HEAD:
            power_ = -0.7;
            HAL::MotorSetPower(HAL::MOTOR_HEAD, power_);
            lastHeadMovedTime_ms = HAL::GetTimeStamp();
            calState_ = HCS_WAIT_FOR_STOP;
            break;
            
          case HCS_WAIT_FOR_STOP:
            // Check for when head stops moving for 0.2 seconds
            if (!IsMoving()) {
              
              if (HAL::GetTimeStamp() - lastHeadMovedTime_ms > HEAD_STOP_TIME) {
                // Turn off motor
                power_ = 0.0;
                HAL::MotorSetPower(HAL::MOTOR_HEAD, power_);
                
                // Set timestamp to be used in next state to wait for motor to "relax"
                lastHeadMovedTime_ms = HAL::GetTimeStamp();
                
                // Go to next state
                calState_ = HCS_SET_CURR_ANGLE;
              }
            } else {
              lastHeadMovedTime_ms = HAL::GetTimeStamp();
            }
            break;
            
          case HCS_SET_CURR_ANGLE:
            // Wait for motor to relax and then set angle
            if (HAL::GetTimeStamp() - lastHeadMovedTime_ms > HEAD_STOP_TIME) {
              PRINT("HEAD Calibrated\n");
              ResetLowAnglePosition();
              calState_ = HCS_IDLE;
            }
            break;
        }
      }
      
    }

    
    f32 GetAngleRad()
    {
      return currentAngle_.ToFloat();
    }
    
    void SetAngleRad(f32 angle)
    {
      currentAngle_ = angle;
    }

    f32 GetLastCommandedAngle()
    {
      return desiredAngle_.ToFloat();
    }

    void GetCamPose(f32 &x, f32 &z, f32 &angle)
    {
      f32 cosH = cosf(currentAngle_.ToFloat());
      f32 sinH = sinf(currentAngle_.ToFloat());
      
      x = HEAD_CAM_POSITION[0]*cosH - HEAD_CAM_POSITION[2]*sinH + NECK_JOINT_POSITION[0];
      z = HEAD_CAM_POSITION[2]*cosH + HEAD_CAM_POSITION[0]*sinH + NECK_JOINT_POSITION[2];
      angle = currentAngle_.ToFloat();
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
  
    f32 GetAngularVelocity()
    {
      return radSpeed_;
    }
    
    void SetAngularVelocity(const f32 rad_per_sec)
    {
      // TODO: Figure out power-to-speed ratio on actual robot. Normalize with battery power?
      f32 power = CLIP(rad_per_sec / HAL::MAX_HEAD_SPEED, -1.0, 1.0);
      HAL::MotorSetPower(HAL::MOTOR_HEAD, power);
      inPosition_ = true;
      
      /*
      // Command a target angle based on the sign of the desired speed
      f32 targetAngle = 0;
      if (rad_per_sec > 0) {
        targetAngle = MAX_HEAD_ANGLE;
        maxSpeedRad_ = rad_per_sec;
      } else if (rad_per_sec < 0) {
        targetAngle = MIN_HEAD_ANGLE;
        maxSpeedRad_ = rad_per_sec;
      } else {
        // Compute the expected height if we were to start slowing down now
        f32 radToStop = 0.5f*(radSpeed_*radSpeed_) / accelRad_;
        if (radSpeed_ < 0) {
          radToStop *= -1;
        }
        targetAngle = currentAngle_.ToFloat() + radToStop;
      }
      SetDesiredAngle(targetAngle);
      */
    }
  
    void SetMaxSpeedAndAccel(const f32 max_speed_rad_per_sec, const f32 accel_rad_per_sec2)
    {
      maxSpeedRad_ = ABS(max_speed_rad_per_sec);
      accelRad_ = accel_rad_per_sec2;
    }
    
    void GetMaxSpeedAndAccel(f32 &max_speed_rad_per_sec, f32 &accel_rad_per_sec2)
    {
      max_speed_rad_per_sec = maxSpeedRad_;
      accel_rad_per_sec2 = accelRad_;
    }
    
    void SetDesiredAngle(f32 angle) {
      SetDesiredAngle(angle, DEFAULT_START_ACCEL_FRAC, DEFAULT_END_ACCEL_FRAC, 0);
    }

    // TODO: There is common code with the other SetDesiredAngle() that can be pulled out into a shared function.
    static void SetDesiredAngle_internal(f32 angle, f32 acc_start_frac, f32 acc_end_frac, f32 duration_seconds)
    {
      // Do range check on angle
      angle = CLIP(angle, MIN_HEAD_ANGLE, MAX_HEAD_ANGLE);
      
      // Check if already at desired angle
      if (inPosition_ &&
          (angle == desiredAngle_) &&
          (ABS((desiredAngle_ - currentAngle_).ToFloat()) < ANGLE_TOLERANCE) ) {
        #if(DEBUG_HEAD_CONTROLLER)
        PRINT("HEAD: Already at desired angle %f degrees\n", RAD_TO_DEG_F32(angle));
        #endif
        return;        
      }
      
      
      desiredAngle_ = angle;
      angleError_ = desiredAngle_.ToFloat() - currentAngle_.ToFloat();
      prevAngleError_ = 0;
      
      
#if(DEBUG_HEAD_CONTROLLER)
      PRINT("HEAD (fixedDuration): SetDesiredAngle %f rads (duration %f)\n", desiredAngle_.ToFloat(), duration_seconds);
#endif
      
      f32 startRadSpeed = radSpeed_;
      f32 startRad = currentAngle_.ToFloat();
      if (!inPosition_) {
        vpg_.Step(startRadSpeed, startRad);
      } else {
        startRadSpeed = 0;
        angleErrorSum_ = 0.f;
      }
      
      lastInPositionTime_ms_ = 0;
      inPosition_ = false;
      
      if (FLT_NEAR(angleError_,0.f)) {
        inPosition_ = true;
#if(DEBUG_HEAD_CONTROLLER)
        PRINT("Head (fixedDuration): Already at desired position\n");
#endif
        return;
      }

      bool res = false;
      // Start profile of head trajectory
      if (duration_seconds > 0) {
        res = vpg_.StartProfile_fixedDuration(startRad, startRadSpeed, acc_start_frac*duration_seconds,
                                              desiredAngle_.ToFloat(), acc_end_frac*duration_seconds,
                                              MAX_HEAD_SPEED_RAD_PER_S,
                                              MAX_HEAD_ACCEL_RAD_PER_S2,
                                              duration_seconds,
                                              CONTROL_DT);
      
        if (!res) {
          PRINT("FAIL: HEAD VPG (fixedDuration): startVel %f, startPos %f, acc_start_frac %f, acc_end_frac %f, endPos %f, duration %f.  Trying VPG without fixed duration.\n",
                startRadSpeed, startRad, acc_start_frac, acc_end_frac, desiredAngle_.ToFloat(), duration_seconds);
        }
      }
      if (!res) {
        //SetDesiredAngle_internal(angle);
        // Start profile of head trajectory
        vpg_.StartProfile(startRadSpeed, startRad,
                          maxSpeedRad_, accelRad_,
                          0, desiredAngle_.ToFloat(),
                          CONTROL_DT);
      }

#if(DEBUG_HEAD_CONTROLLER)
      PRINT("HEAD VPG (fixedDuration): startVel %f, startPos %f, acc_start_frac %f, acc_end_frac %f, endPos %f, duration %f\n",
            startRadSpeed, startRad, acc_start_frac, acc_end_frac, desiredAngle_.ToFloat(), duration_seconds);
#endif

    } // SetDesiredAngle_internal()
  
    void SetDesiredAngle(f32 angle, f32 acc_start_frac, f32 acc_end_frac, f32 duration_seconds)
    {
      // Stop nodding if we were
      if(IsNodding()) {
        isNodding_ = false;
      }
    
      SetDesiredAngle_internal(angle, acc_start_frac, acc_end_frac, duration_seconds);
    }
  
  
    bool IsInPosition(void) {
      return inPosition_;
    }
    
    Result Update()
    {
      CalibrationUpdate();
      
      PoseAndSpeedFilterUpdate();
      
      if (!enable_) {
        return RESULT_OK;
      }
      
      // Note that a new call to SetDesiredAngle will get
      // Update() working again after it has reached a previous
      // setting.
      if(not inPosition_) {

        // Get the current desired head angle
        f32 currDesiredRadVel;
        vpg_.Step(currDesiredRadVel, currDesiredAngle_);
        
        // Compute current angle error
        angleError_ = currDesiredAngle_ - currentAngle_.ToFloat();
        
        // Compute power value
        power_ = (Kp_ * angleError_) + (Kd_ * (angleError_ - prevAngleError_) * CONTROL_DT) + (Ki_ * angleErrorSum_);
        
        // Add base power in the direction of the desired general direction
        if (power_ > 0) {
          power_ += BASE_POWER;
        } else if (power_ < 0) {
          power_ -= BASE_POWER;
        }
        
        // Update angle error sum
        prevAngleError_ = angleError_;
        angleErrorSum_ += angleError_;
        angleErrorSum_ = CLIP(angleErrorSum_, -MAX_ERROR_SUM, MAX_ERROR_SUM);
        

        // If accurately tracking current desired angle...
        if(((ABS(angleError_) < ANGLE_TOLERANCE) && (desiredAngle_ == currDesiredAngle_))) {
          
          if (lastInPositionTime_ms_ == 0) {
            lastInPositionTime_ms_ = HAL::GetTimeStamp();
          } else if (HAL::GetTimeStamp() - lastInPositionTime_ms_ > IN_POSITION_TIME_MS) {
            
            power_ = 0;
            inPosition_ = true;
            
#         if(DEBUG_HEAD_CONTROLLER)
            PRINT(" HEAD ANGLE REACHED (%f rad)\n", GetAngleRad() );
#         endif
          }
        } else {
          lastInPositionTime_ms_ = 0;
        }

        
        
        
#       if(DEBUG_HEAD_CONTROLLER)
        PERIODIC_PRINT(100, "HEAD: currA %f, curDesA %f, desA %f, err %f, errSum %f, pwr %f, spd %f\n",
                       currentAngle_.ToFloat(),
                       currDesiredAngle_,
                       desiredAngle_.ToFloat(),
                       angleError_,
                       angleErrorSum_,
                       power_,
                       radSpeed_);
        PERIODIC_PRINT(100, "  POWER terms: %f  %f\n", (Kp_ * angleError_), (Ki_ * angleErrorSum_))
#       endif
        
        power_ = CLIP(power_, -1.0, 1.0);
        
        
        HAL::MotorSetPower(HAL::MOTOR_HEAD, power_);
      } // if not in position
      else if(isNodding_)
      { // inPosition and Nodding
        if (GetLastCommandedAngle() == nodHighAngle_) {
          SetDesiredAngle_internal(nodLowAngle_, nodEaseOutFraction_, nodEaseInFraction_, nodHalfPeriod_sec_);
        } else if (GetLastCommandedAngle() == nodLowAngle_) {
          SetDesiredAngle_internal(nodHighAngle_, nodEaseOutFraction_, nodEaseInFraction_, nodHalfPeriod_sec_);
          ++numNodsComplete_;
          if(numNodsDesired_ > 0 && numNodsComplete_ >= numNodsDesired_) {
            StopNodding();
          }
        }
      } // else if(isNodding)
      
      return RESULT_OK;
    }
    
    void SetGains(const f32 kp, const f32 ki, const f32 kd, const f32 maxIntegralError)
    {
      Kp_ = kp;
      Ki_ = ki;
      Kd_ = kd;
      MAX_ERROR_SUM = maxIntegralError;
      PRINT("New head gains: kp = %f, ki = %f, kd = %f, maxSum = %f\n",
            Kp_, Ki_, Kd_, MAX_ERROR_SUM);
    }
  
    void StartNodding(const f32 lowAngle, const f32 highAngle,
                      const u16 period_ms, const s32 numLoops,
                      const f32 easeInFraction, const f32 easeOutFraction)
    {
      //AnkiConditionalErrorAndReturnValue(keyFrame.type != KeyFrame::HEAD_NOD, RESULT_FAIL, "HeadNodStart.WrongKeyFrameType", "\n");

      AnkiConditionalWarnAndReturn(enable_, "HeadController.StartNodding.Disabled",
                                   "StartNodding() command ignored: HeadController is disabled.\n");
      
      //preNodAngle_ = GetAngleRad();
      nodLowAngle_  = lowAngle;
      nodHighAngle_ = highAngle;
      
      numNodsDesired_  = numLoops;
      numNodsComplete_ = 0;
      isNodding_ = true;
      nodEaseOutFraction_ = easeOutFraction;
      nodEaseInFraction_  = easeInFraction;
      
      nodHalfPeriod_sec_ = static_cast<f32>(period_ms) * .5f * 0.001f;
      SetDesiredAngle_internal(nodLowAngle_, nodEaseOutFraction_, nodEaseInFraction_, nodHalfPeriod_sec_);
      
    } // StartNodding()
    
    
    void StopNodding()
    {
      AnkiConditionalWarnAndReturn(enable_, "HeadController.StopNodding.Disabled",
                                   "StopNodding() command ignored: HeadController is disabled.\n");
      
      //SetDesiredAngle_internal(preNodAngle_);
      isNodding_ = false;
    }
    
    bool IsNodding()
    {
      return isNodding_;
    }
    
  } // namespace HeadController
  } // namespace Cozmo
} // namespace Anki
