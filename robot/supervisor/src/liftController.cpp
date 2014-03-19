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
        
        // How long the lift needs to stop moving for before it is considered to be limited.
#ifdef SIMULATOR
        const u32 LIFT_STOP_TIME = 200000;
#else
        const u32 LIFT_STOP_TIME = 2000000;
#endif
        
        // Re-calibrates lift position whenever LIFT_HEIGHT_LOW is commanded.
        const bool RECALIBRATE_AT_LOW_HEIGHT = false;
        
        // If lift comes within this distance to limit angle, trigger recalibration.
        const f32 RECALIBRATE_LIMIT_ANGLE_THRESH = 0.1f;
        
        const f32 MAX_LIFT_CONSIDERED_STOPPED_RAD_PER_SEC = 0.001;
        
        const f32 SPEED_FILTERING_COEFF = 0.9f;
        
        
        const f32 Kp_ = 0.5f; // proportional control constant
        const f32 Ki_ = 0.02f; // integral control constant
        f32 angleErrorSum_ = 0.f;
        const f32 MAX_ERROR_SUM = 200.f;
        
        const f32 ANGLE_TOLERANCE = DEG_TO_RAD(0.5f);
        f32 LIFT_ANGLE_LOW; // Initialize in Init()

        // Open loop gain
        // power_open_loop = SPEED_TO_POWER_OL_GAIN * desiredSpeed + BASE_POWER
        const f32 SPEED_TO_POWER_OL_GAIN = 0.2399;
        const f32 BASE_POWER_UP = 0.1309;
        const f32 BASE_POWER_DOWN = -0.0829;
        
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
        f32 approachSpeedRad_ = 0.2f;
        
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
      
      
      void ResetLowAnglePosition()
      {
        currentAngle_ = LIFT_ANGLE_LOW;
        HAL::MotorResetPosition(HAL::MOTOR_LIFT);
        prevHalPos_ = HAL::MotorGetPosition(HAL::MOTOR_LIFT);
        isCalibrated_ = true;
      }
      
      void StartCalibrationRoutine()
      {
        PRINT("Starting Lift calibration\n");
        
#ifdef SIMULATOR
        // Skipping actual calibration routine in sim due to weird lift behavior when attempting to move it when
        // it's at the joint limit.  The arm flies off the robot!
        ResetLowAnglePosition();
#else
        calState_ = LCS_LOWER_LIFT;
        isCalibrated_ = false;
#endif
      }
      
      bool IsCalibrated()
      {
        return isCalibrated_;
      }
      

      bool IsMoving()
      {
        return (ABS(radSpeed_) > MAX_LIFT_CONSIDERED_STOPPED_RAD_PER_SEC);
      }
      
      void CalibrationUpdate()
      {
        if (!isCalibrated_) {
          
          switch(calState_) {
              
            case LCS_IDLE:
              break;
              
            case LCS_LOWER_LIFT:
              power_ = -0.3;
              HAL::MotorSetPower(HAL::MOTOR_LIFT, power_);
              lastLiftMovedTime_us = HAL::GetMicroCounter();
              calState_ = LCS_WAIT_FOR_STOP;
              break;
              
            case LCS_WAIT_FOR_STOP:
              // Check for when lift stops moving for 0.2 seconds
              if (!IsMoving()) {

                if (HAL::GetMicroCounter() - lastLiftMovedTime_us > LIFT_STOP_TIME) {
                  // Turn off motor
                  power_ = 0;  // Not strong enough to lift motor, but just enough to unwind backlash. Not sure if this is actually helping.
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
                PRINT("LIFT Calibrated\n");
                power_ = 0;
                HAL::MotorSetPower(HAL::MOTOR_LIFT, power_);
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
      
      
      void SetDesiredHeight(f32 height_mm)
      {
        
        // Do range check on height
        height_mm = CLIP(height_mm, LIFT_HEIGHT_LOWDOCK, LIFT_HEIGHT_CARRY);
        
        
        // Convert desired height into the necessary angle:
#if(DEBUG_LIFT_CONTROLLER)
        PRINT("LIFT DESIRED HEIGHT: %f mm (curr height %f mm)\n", height_mm, GetHeightMM());
#endif
        desiredAngle_ = Height2Rad(height_mm);
        angleError_ = desiredAngle_.ToFloat() - currentAngle_.ToFloat();
        angleErrorSum_ = 0.f;
        
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
        PRINT("LIFT VPG: startVel %f, startPos %f, maxVel %f, accel %f, endVel %f, endPos %f\n",
              radSpeed_, currentAngle_.ToFloat(), maxSpeedRad_, accelRad_, approachSpeedRad_, desiredAngle_.ToFloat());
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
          
          // Get the current desired lift angle
          vpg_.Step(currDesiredRadVel_, currDesiredAngle_);
          
          // Compute position error
          angleError_ = currDesiredAngle_ - currentAngle_.ToFloat();
          
          // Open loop value to drive at desired speed
          power_ = currDesiredRadVel_ * SPEED_TO_POWER_OL_GAIN;
          
          // Compute corrective value
          f32 power_corr = (Kp_ * angleError_) + (Ki_ * angleErrorSum_);
          
          // Add base power in the direction of corrective value
          power_ += power_corr + ((power_corr > 0) ? BASE_POWER_UP : BASE_POWER_DOWN);
          
          // Update angle error sum
          angleErrorSum_ += angleError_;
          angleErrorSum_ = CLIP(angleErrorSum_, -MAX_ERROR_SUM, MAX_ERROR_SUM);

          // If accurately tracking current desired angle...
          if((ABS(angleError_) < ANGLE_TOLERANCE && desiredAngle_ == currDesiredAngle_)
             || ABS(currentAngle_ - desiredAngle_) < ANGLE_TOLERANCE) {
              power_ = 0.f;
              inPosition_ = true;
              #if(DEBUG_LIFT_CONTROLLER)
              PRINT(" LIFT HEIGHT REACHED (%f mm)\n", GetHeightMM());
              #endif
          }
          
          
          // TODO: Incomplete calibration code.
          //       If quadrature is good, we don't need this!
          /*
          // If accurately tracking current desired angle...
          if(ABS(angleError_) < ANGLE_TOLERANCE) {
            angleErrorSum_ = 0.f;
            
            // If desired angle is low position, let it fall through to recalibration.
            // Otherwise, if desired angle has been reached, set inPosition=true;
            if (!(RECALIBRATE_AT_LOW_HEIGHT && desiredAngle_.ToFloat() == LIFT_ANGLE_LOW)) {
              power_ = 0.f;
              
              if (desiredAngle_ == currDesiredAngle_) {
                inPosition_ = true;
#if(DEBUG_LIFT_CONTROLLER)
                PRINT(" LIFT HEIGHT REACHED (%f mm)\n", GetHeightMM());
#endif
              }
            }
          }
          */


#if(DEBUG_LIFT_CONTROLLER)
          PERIODIC_PRINT(100, "LIFT: currA %f, curDesA %f, currVel %f, curDesVel %f, desA %f, err %f, errSum %f, pwr %f\n",
                         currentAngle_.ToFloat(),
                         currDesiredAngle_,
                         radSpeed_,
                         currDesiredRadVel_,
                         desiredAngle_.ToFloat(),
                         angleError_,
                         angleErrorSum_,
                         power_);
          PERIODIC_PRINT(100, "  POWER terms: %f  %f\n", (Kp_ * angleError_), (Ki_ * angleErrorSum_))
#endif
          
          power_ = CLIP(power_, -1.0, 1.0);
          
/*
          // If within 5 degrees of LIFT_HEIGHT_LOW and the lift isn't moving while downward power is applied,
          // assume we've hit the limit and recalibrate.
          if (limitingDetected_ ||
              ((power_ < 0)
              && (desiredAngle_.ToFloat() == LIFT_ANGLE_LOW)
              && (LIFT_ANGLE_LOW == currDesiredAngle_)
              && (ABS(angleError_) < RECALIBRATE_LIMIT_ANGLE_THRESH)
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
          HAL::MotorSetPower(HAL::MOTOR_LIFT, power_);
          
        } // if not in position
        
        return EXIT_SUCCESS;
      }
    } // namespace LiftController
  } // namespace Cozmo
} // namespace Anki
