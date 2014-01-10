#include "liftController.h"
#include "anki/common/robot/config.h"
#include "anki/common/shared/velocityProfileGenerator.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/common/robot/utilities_c.h"
#include "anki/common/shared/radians.h"



namespace Anki {
  namespace Cozmo {
    namespace LiftController {
      
      
      namespace {
        
        // HACK: For use on robots with no position sensors.
        // Tuned for physical robot only, not simulation robot.
#ifdef SIMULATOR
        #define OPEN_LOOP_LIFT_CONTROL 0
#else
        #define OPEN_LOOP_LIFT_CONTROL 1
#endif
        
        
#if(OPEN_LOOP_LIFT_CONTROL)
        f32 currHeight_ = LIFT_HEIGHT_LOWDOCK;
        f32 desiredHeight_;
        u32 liftStopTimeUS_;
#endif
        
        // Re-calibrates lift position whenever LIFT_HEIGHT_LOW is commanded.
        #define RECALIBRATE_AT_LOW_HEIGHT 0
        
        const f32 MAX_LIFT_CONSIDERED_STOPPED_RAD_PER_SEC = 0.001;
        
        const f32 SPEED_FILTERING_COEFF = 0.9f;
        
        const f32 Kp_ = 0.5f; // proportional control constant
        const f32 Ki_ = 0.001f; // integral control constant
        f32 angleErrorSum_ = 0.f;
        const f32 MAX_ERROR_SUM = 200.f;
        
        const f32 ANGLE_TOLERANCE = DEG_TO_RAD(0.5f);
        f32 LIFT_ANGLE_LOW; // Initialize in Init()
        
        // Minimum power required to move lift (no load)
        const f32 MIN_POWER = 0.3;
        f32 minPower_ = 0.f;
        
        // Angle of the main lift arm.
        // On the real robot, this is the angle between the lower lift joint on the robot body
        // and the lower lift joint on the forklift assembly.
        Radians currentAngle_ = 0.f;
        Radians desiredAngle_ = 0.f;
        f32 currDesiredAngle_ = 0.f;
        f32 currDesiredRadVel_ = 0.f;
        f32 angleError_ = 0.f;
        f32 prevHalPos_ = 0.f;
        bool inPosition_  = true;
        
        // Speed and acceleration params
        f32 maxSpeedRad_ = 1.0f;
        f32 accelRad_ = 2.0f;
        f32 approachSpeedRad_ = 0.1f;
        
        // For generating position and speed profile
        VelocityProfileGenerator vpg_;
        
        // Current speed
        f32 radSpeed_ = 0;
        
        // Currently applied power
        f32 power_ = 0;

        
        // Calibration parameters
        typedef enum {
          LCS_IDLE,
          LCS_LOWER_LIFT,
          LCS_WAIT_FOR_STOP,
          LCS_SET_CURR_ANGLE
        } LiftCalibState;
        
        LiftCalibState calState_ = LCS_IDLE;
        bool doCalib_ = false;
        bool isCalibrated_ = false;
        bool limitingDetected_ = false;
        u32 lastLiftMovedTime_us = 0;
        
        // Whether or not to command anything to motor
        bool enable_ = true;
        
      } // "private" members

      f32 Height2Rad(f32 height_mm) {
        return asinf((height_mm - LIFT_JOINT_HEIGHT - LIFT_FORK_HEIGHT_REL_TO_ARM_END)/LIFT_ARM_LENGTH);
      }
      
      f32 Rad2Height(f32 angle) {
        return (sinf(angle) * LIFT_ARM_LENGTH) + LIFT_JOINT_HEIGHT + LIFT_FORK_HEIGHT_REL_TO_ARM_END;
      }
      
      
      ReturnCode Init()
      {
        // Init consts
        LIFT_ANGLE_LOW = Height2Rad(LIFT_HEIGHT_LOWDOCK);
        return EXIT_SUCCESS;
      }
      
      
      void Enable()
      {
        enable_ = true;
      }
      
      void Disable()
      {
        enable_ = false;
      }
      
      
      void StartCalibrationRoutine()
      {
        PRINT("Starting Lift calibration\n");
#ifdef SIMULATOR
        // Skipping actual calibration routine in sim due to weird lift behavior when attempting to move it when
        // it's at the joint limit.  The arm flies off the robot!
        isCalibrated_ = true;
#else
        calState_ = LCS_LOWER_LIFT;
        isCalibrated_ = false;
#endif
      }
      
      bool IsCalibrated()
      {
        return isCalibrated_;
      }
      
      
      void ResetLowAnglePosition()
      {
        currentAngle_ = LIFT_ANGLE_LOW;
        HAL::MotorResetPosition(HAL::MOTOR_LIFT);
        prevHalPos_ = HAL::MotorGetPosition(HAL::MOTOR_LIFT);
        doCalib_ = false;
        isCalibrated_ = true;
      }
      
      void CalibrationUpdate()
      {
        if (!isCalibrated_) {
          
          switch(calState_) {
              
            case LCS_IDLE:
              break;
              
            case LCS_LOWER_LIFT:
              power_ = -0.4;
              HAL::MotorSetPower(HAL::MOTOR_LIFT, power_);
              lastLiftMovedTime_us = HAL::GetMicroCounter();
              calState_ = LCS_WAIT_FOR_STOP;
              break;
              
            case LCS_WAIT_FOR_STOP:
              // Check for when lift stops moving for 0.2 seconds
              if (ABS(HAL::MotorGetSpeed(HAL::MOTOR_LIFT)) < MAX_LIFT_CONSIDERED_STOPPED_RAD_PER_SEC) {
#ifdef SIMULATOR
                const u32 LIFT_STOP_TIME = 200000;
#else
                const u32 LIFT_STOP_TIME = 2000000;
#endif
                if (HAL::GetMicroCounter() - lastLiftMovedTime_us > LIFT_STOP_TIME) {
                  // Turn off motor
                  power_ = 0.0;
                  HAL::MotorSetPower(HAL::MOTOR_LIFT, power_);
                  
                  // Set timestamp to be used in next state to wait for motor to "relax"
                  lastLiftMovedTime_us = HAL::GetMicroCounter();
                  
                  // Go to next state
                  calState_ = LCS_SET_CURR_ANGLE;
                }
              } else {
                lastLiftMovedTime_us = HAL::GetMicroCounter();
              }
              break;
              
            case LCS_SET_CURR_ANGLE:
              // Wait for motor to relax and then set angle
              if (HAL::GetMicroCounter() - lastLiftMovedTime_us > 200000) {
                ResetLowAnglePosition();
                calState_ = LCS_IDLE;
              }
              break;
          }
        }
        
      }

      f32 GetHeightMM()
      {
        return Rad2Height(currentAngle_.ToFloat());
      }
      
      f32 GetAngleRad()
      {
        return currentAngle_.ToFloat();
      }
      
      void SetSpeedAndAccel(f32 max_speed_rad_per_sec, f32 accel_rad_per_sec2)
      {
        maxSpeedRad_ = max_speed_rad_per_sec;
        accelRad_ = accel_rad_per_sec2;
      }
      
      
      void SetAngularVelocity(const f32 rad_per_sec)
      {
        // TODO: Figure out power-to-speed ratio on actual robot. Normalize with battery power?
        power_ = CLIP(rad_per_sec / HAL::MAX_LIFT_SPEED, -1.0, 1.0);
        HAL::MotorSetPower(HAL::MOTOR_LIFT, power_);
        inPosition_ = true;
      }

      f32 GetAngularVelocity()
      {
        return radSpeed_;
      }
      
      void PoseAndSpeedFilterUpdate()
      {
        // Get encoder speed measurements
        f32 measuredSpeed = Cozmo::HAL::MotorGetSpeed(HAL::MOTOR_LIFT);
        
        radSpeed_ = (measuredSpeed *
                     (1.0f - SPEED_FILTERING_COEFF) +
                     (radSpeed_ * SPEED_FILTERING_COEFF));
        
        // Update position
        currentAngle_ += (HAL::MotorGetPosition(HAL::MOTOR_LIFT) - prevHalPos_);
        
#if(DEBUG_LIFT_CONTROLLER)
        PRINT("LIFT FILT: speed %f, speedFilt %f, currentAngle %f, currHalPos %f, prevPos %f, pwr %f\n",
              measuredSpeed, radSpeed_, currentAngle_.ToFloat(), HAL::MotorGetPosition(HAL::MOTOR_LIFT), prevHalPos_, power_);
#endif
        prevHalPos_ = HAL::MotorGetPosition(HAL::MOTOR_LIFT);
      }
      
      
      void SetDesiredHeight(const f32 height_mm)
      {
        
#if(OPEN_LOOP_LIFT_CONTROL)
        desiredHeight_ = height_mm;
        inPosition_ = false;
        power_ = 0.f;
        f32 liftMotionTime = 0;
        
        if (currHeight_ == LIFT_HEIGHT_LOWDOCK) {
          if (desiredHeight_ == LIFT_HEIGHT_CARRY) {
            liftMotionTime = 2.8;      // with block on lift
            power_ = 0.5;
          } else if (desiredHeight_ == LIFT_HEIGHT_HIGHDOCK) {
            liftMotionTime = 2.5;
            power_ = 0.5;
          } else {
            inPosition_ = true;
          }
        }
        else if (currHeight_ == LIFT_HEIGHT_HIGHDOCK) {
          if (desiredHeight_ == LIFT_HEIGHT_CARRY) {
            liftMotionTime = 0.5; // with block on lift
            power_ = 0.5;
          } else if (desiredHeight_ == LIFT_HEIGHT_LOWDOCK) {
            liftMotionTime = 2;
            power_ = -0.3;
          } else {
            inPosition_ = true;
          }
        }
        else if (currHeight_ == LIFT_HEIGHT_CARRY) {
          if (desiredHeight_ == LIFT_HEIGHT_LOWDOCK) {
            liftMotionTime = 3;
            power_ = -0.3;
          } else if (desiredHeight_ == LIFT_HEIGHT_HIGHDOCK) {
            liftMotionTime = 0.33;  // with block on lift
            power_ = -0.3;
          } else {
            inPosition_ = true;
          }
        }
        
        liftStopTimeUS_ = HAL::GetMicroCounter() + liftMotionTime * 1000000;
        HAL::MotorSetPower(HAL::MOTOR_LIFT, power_);
        PRINT("LIFT MOVING (POWER = %f, TIME = %f s)\n", power_, liftMotionTime);
        return;
#endif
        
        // Convert desired height into the necessary angle:
#if(DEBUG_LIFT_CONTROLLER)
        PRINT("LIFT DESIRED HEIGHT: %f mm (curr height %f mm)\n", height_mm, GetHeightMM());
#endif
        desiredAngle_ = Height2Rad(height_mm);
        angleError_ = desiredAngle_.ToFloat() - currentAngle_.ToFloat();
        angleErrorSum_ = 0.f;

        // Minimum power required to move the lift
        minPower_ = MIN_POWER;
        if (angleError_ < 0) {
          minPower_ = -MIN_POWER;
        }
        
        limitingDetected_ = false;
        inPosition_ = false;
        
        if (FLT_NEAR(angleError_,0.f)) {
          inPosition_ = true;
          PRINT("Lift: Already at desired position\n");
          return;
        }

        // Start profile of lift trajectory
        vpg_.StartProfile(radSpeed_, currentAngle_.ToFloat(),
                          maxSpeedRad_, accelRad_,
                          approachSpeedRad_, desiredAngle_.ToFloat(),
                          CONTROL_DT);

#if(DEBUG_LIFT_CONTROLLER)
        PRINT("LIFT VPG: startVel %f, startPos %f, maxVel %f, endVel %f, endPos %f\n",
              radSpeed_, currentAngle_.ToFloat(), maxSpeedRad_, approachSpeedRad_, desiredAngle_.ToFloat());
#endif
        
      }
      
      bool IsInPosition(void) {
        return inPosition_;
      }
      
      ReturnCode Update()
      {
        // Update routine for calibration sequence
        CalibrationUpdate();
        
        PoseAndSpeedFilterUpdate();
        
        // If disabled, do not activate motors
        if(!enable_) {
          return EXIT_SUCCESS;
        }
        
        if(not inPosition_) {
          
#if(OPEN_LOOP_LIFT_CONTROL)
          if (HAL::GetMicroCounter() > liftStopTimeUS_) {
            PRINT("STOPPING LIFT\n");
            currHeight_ = desiredHeight_;
            power_ = 0;
            HAL::MotorSetPower(HAL::MOTOR_LIFT, 0);
            inPosition_ = true;
          }
          return EXIT_SUCCESS;
#endif
          
          // Get the current desired lift angle
          vpg_.Step(currDesiredRadVel_, currDesiredAngle_);
#if(DEBUG_LIFT_CONTROLLER)
          PRINT("LIFT desVel: %f, desAng: %f, currAng: %f\n",
                currDesiredRadVel_, currDesiredAngle_, currentAngle_.ToFloat());
#endif
          
          // Simple proportional control for now
          // TODO: better controller?
          angleError_ = currDesiredAngle_ - currentAngle_.ToFloat();
          
          // TODO: convert angleError_ to power / speed in some reasonable way
          if(ABS(angleError_) < ANGLE_TOLERANCE) {
            angleErrorSum_ = 0.f;
            
            // If desired angle is low position, let it fall through to recalibration
            if (!RECALIBRATE_AT_LOW_HEIGHT || desiredAngle_.ToFloat() != LIFT_ANGLE_LOW) {
              power_ = 0.f;
              
              if (desiredAngle_ == currDesiredAngle_) {
                inPosition_ = true;
#if(DEBUG_LIFT_CONTROLLER)
                PRINT(" LIFT HEIGHT REACHED (%f mm)\n", GetHeightMM());
#endif
              }
            }
          } else {
            power_ = minPower_ + (Kp_ * angleError_) + (Ki_ * angleErrorSum_);
            angleErrorSum_ += angleError_;
            angleErrorSum_ = CLIP(angleErrorSum_, -MAX_ERROR_SUM, MAX_ERROR_SUM);
            inPosition_ = false;
          }

#if(DEBUG_LIFT_CONTROLLER)
          PERIODIC_PRINT(100, "LIFT: currA %f, curDesA %f, desA %f, err %f, errSum %f, pwr %f, spd %f\n",
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
          
          HAL::MotorSetPower(HAL::MOTOR_LIFT, power_);
          
        } // if not in position
        
        return EXIT_SUCCESS;
      }
    } // namespace LiftController
  } // namespace Cozmo
} // namespace Anki