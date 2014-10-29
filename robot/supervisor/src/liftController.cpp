#include "liftController.h"
#include "anki/common/robot/config.h"
#include "anki/common/shared/velocityProfileGenerator.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/common/robot/utilities_c.h"
#include "anki/common/shared/radians.h"


#define DEBUG_LIFT_CONTROLLER 0

// Re-calibrates lift position whenever lift is commanded to the limits
#ifdef SIMULATOR
#define RECALIBRATE_AT_LIMITS 0  // Don't do calibration at limits in simulator.
                                 // Hard stops don't seem to be respected in Webots model.
#else
#define RECALIBRATE_AT_LIMITS 1
#endif

namespace Anki {
  namespace Cozmo {
    namespace LiftController {
      
      
      namespace {
        
        // How long the lift needs to stop moving for before it is considered to be limited.
#ifdef SIMULATOR
        const u32 LIFT_STOP_TIME = 200000;
#else
        const u32 LIFT_STOP_TIME = 500000;
#endif
        
        // Amount of time to allow lift to relax with power == 0, before considering it
        // to have settled enough for recalibration.
        const u32 LIFT_RELAX_TIME = 200000;
      
#if RECALIBRATE_AT_LIMITS
        // Power with which to approach limit angle (after the intended velocity profile has been executed)
        // TODO: Shouldn't have to be this strong. Lower when 2.1 version electronics are ready.
        const f32 LIMIT_APPROACH_POWER = 0.7;
#endif
        
        const f32 MAX_LIFT_CONSIDERED_STOPPED_RAD_PER_SEC = 0.001;
        
        const f32 SPEED_FILTERING_COEFF = 0.9f;
        
        
        f32 Kp_ = 20.f; // proportional control constant
        f32 Ki_ = 0.03f; // integral control constant
        f32 angleErrorSum_ = 0.f;
        f32 MAX_ERROR_SUM = 10.f;
        
        const f32 ANGLE_TOLERANCE = DEG_TO_RAD(0.5f);
        f32 LIFT_ANGLE_LOW_LIMIT; // Initialize in Init()
        f32 LIFT_ANGLE_HIGH_LIMIT; // Initialize in Init()
        f32 LIFT_ANGLE_HIGH_DOCK;

#ifdef SIMULATOR
        // For disengaging gripper once the lift has reached its final position
        bool disengageGripperAtDest_ = false;
        f32  disengageAtAngle_ = 0.f;
#endif
        
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
        f32 desiredHeight_ = 0.f;
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

        // Nodding
        //f32 preNodHeight_    = 0.f;
        f32 nodLowHeight_    = 0.f;
        f32 nodHighHeight_   = 0.f;
        s32 numNodsDesired_  = 0;
        s32 numNodsComplete_ = 0;
        bool isNodding_ = false;

        
        // Calibration parameters
        typedef enum {
          LCS_IDLE,
          LCS_LOWER_LIFT,
          LCS_WAIT_FOR_STOP,
          LCS_SET_CURR_ANGLE
        } LiftCalibState;
        
#ifndef SIMULATOR
        LiftCalibState calState_ = LCS_IDLE;
#endif
        bool isCalibrated_ = false;
        bool limitingDetected_ = false;
        bool limitingExpected_ = false;
        bool isRelaxed_ = true;   // LIFT_RELAX_TIME has pass since power stopped being applied

        u32 lastLiftMovedTime_us = 0;
        u32 lastPowerAppliedTime_us = 0;
        
        bool calibPending_ = false;
        f32 calibAngle_;
        
        // Whether or not to command anything to motor
        bool enable_ = true;
        
      } // "private" members

      // Returns the angle between the shoulder joint and the wrist joint.
      f32 Height2Rad(f32 height_mm) {
        height_mm = CLIP(height_mm, LIFT_HEIGHT_LOWDOCK, LIFT_HEIGHT_CARRY);
        return asinf((height_mm - LIFT_BASE_POSITION[2] - LIFT_FORK_HEIGHT_REL_TO_ARM_END)/LIFT_ARM_LENGTH);
      }
      
      f32 Rad2Height(f32 angle) {
        return (sinf(angle) * LIFT_ARM_LENGTH) + LIFT_BASE_POSITION[2] + LIFT_FORK_HEIGHT_REL_TO_ARM_END;
      }
      
      
      Result Init()
      {
        // Init consts
        LIFT_ANGLE_LOW_LIMIT = Height2Rad(LIFT_HEIGHT_LOWDOCK);
        LIFT_ANGLE_HIGH_LIMIT = Height2Rad(LIFT_HEIGHT_CARRY);
        LIFT_ANGLE_HIGH_DOCK = Height2Rad(LIFT_HEIGHT_HIGHDOCK);
        return RESULT_OK;
      }
      
      
      void Enable()
      {
        enable_ = true;
      }
      
      void Disable()
      {
        enable_ = false;
      }
      
      
      void ResetAnglePosition(f32 currAngle)
      {
        currentAngle_ = currAngle;
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
        ResetAnglePosition(LIFT_ANGLE_LOW_LIMIT);
        desiredHeight_ = LIFT_HEIGHT_LOWDOCK;
#else
        
#if(RECALIBRATE_AT_LIMITS)
        SetDesiredHeight(LIFT_HEIGHT_LOWDOCK);
#else
        calState_ = LCS_LOWER_LIFT;
        isCalibrated_ = false;
#endif
        
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

#if (!defined(SIMULATOR) && (RECALIBRATE_AT_LIMITS == 0))
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
              if (HAL::GetMicroCounter() - lastLiftMovedTime_us > LIFT_RELAX_TIME) {
                PRINT("LIFT Calibrated\n");
                power_ = 0;
                HAL::MotorSetPower(HAL::MOTOR_LIFT, power_);
                ResetAnglePosition(LIFT_ANGLE_LOW_LIMIT);
                calState_ = LCS_IDLE;
              }
              break;
          }
        }
        
      }
#endif

      f32 GetLastCommandedHeightMM()
      {
        return desiredHeight_;
      }
      
      f32 GetHeightMM()
      {
        return Rad2Height(currentAngle_.ToFloat());
      }
      
      f32 GetAngleRad()
      {
        return currentAngle_.ToFloat();
      }
      
      void SetSpeedAndAccel(const f32 max_speed_rad_per_sec, const f32 accel_rad_per_sec2)
      {
        maxSpeedRad_ = max_speed_rad_per_sec;
        accelRad_ = accel_rad_per_sec2;
      }
      
      void GetSpeedAndAccel(f32 &max_speed_rad_per_sec, f32 &accel_rad_per_sec2)
      {
        max_speed_rad_per_sec = maxSpeedRad_;
        accel_rad_per_sec2 = accelRad_;
      }
      
      void SetLinearVelocity(const f32 mm_per_sec)
      {
        const f32 rad_per_sec = Height2Rad(mm_per_sec);
        SetAngularVelocity(rad_per_sec);
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
      
      
      bool IsLimiting(f32* limitAngle)
      {
        if (limitingDetected_ && limitAngle) {
          *limitAngle = power_ > 0 ? LIFT_ANGLE_HIGH_LIMIT : LIFT_ANGLE_LOW_LIMIT;
        }
        return limitingDetected_;
      }
      
      bool IsRelaxed()
      {
        return isRelaxed_;
      }
      
      void LimitDetectUpdate()
      {
        if (IsMoving()) {
          lastLiftMovedTime_us = HAL::GetMicroCounter();
          limitingDetected_ = false;
        } else if (ABS(power_) > 0 && HAL::GetMicroCounter() - lastLiftMovedTime_us > LIFT_STOP_TIME) {
          limitingDetected_ = true;
        }
        
        if (power_ == 0.f) {
          if (HAL::GetMicroCounter() - lastPowerAppliedTime_us > LIFT_RELAX_TIME) {
            isRelaxed_ = true;
          }
        } else {
          lastPowerAppliedTime_us = HAL::GetMicroCounter();
          isRelaxed_ = false;
        }
        
        
#if(DEBUG_LIFT_CONTROLLER)
        static bool wasLimiting = false;
        if (!wasLimiting && limitingDetected_) {
          PRINT("Lift LIMITED\n");
          wasLimiting = true;
        } else if (wasLimiting && !limitingDetected_) {
          PRINT("Lift FREE\n");
          wasLimiting = false;
        }
        
        static bool wasRelaxed = true;
        if (!wasRelaxed && isRelaxed_) {
          PRINT("Lift RELAXED\n");
          wasRelaxed = true;
        } else if (wasRelaxed && !isRelaxed_) {
          PRINT("Lift ENGAGED\n");
          wasRelaxed = false;
        }
#endif
        
      }
      
      
      static void SetDesiredHeight_internal(f32 height_mm)
      {
        
        // Do range check on height
        const f32 newDesiredHeight = CLIP(height_mm, LIFT_HEIGHT_LOWDOCK, LIFT_HEIGHT_CARRY);

        // Exit early if lift is already moving towards the commanded height
        if (desiredHeight_ == newDesiredHeight && !inPosition_) {
          return;
        }

#ifdef SIMULATOR
        if(!HAL::IsGripperEngaged()) {
          // If the new desired height will make the lift move upward, turn on
          // the gripper's locking mechanism so that we might pick up a block as
          // it goes up
          if(newDesiredHeight > desiredHeight_) {
            HAL::EngageGripper();
          }
        }
        else {
          // If we're moving the lift down and the end goal is at low-place or
          // high-place height, disengage the gripper when we get there
          if(newDesiredHeight < desiredHeight_ &&
             (newDesiredHeight == LIFT_HEIGHT_LOWDOCK ||
              newDesiredHeight == LIFT_HEIGHT_HIGHDOCK))
          {
            disengageGripperAtDest_ = true;
            disengageAtAngle_ = Height2Rad(newDesiredHeight + 3.f*LIFT_FINGER_HEIGHT);
          }
          else {
            disengageGripperAtDest_ = false;
          }
        }
#endif
           
        desiredHeight_ = newDesiredHeight;
        
        // Convert desired height into the necessary angle:
#if(DEBUG_LIFT_CONTROLLER)
        PRINT("LIFT DESIRED HEIGHT: %f mm (curr height %f mm)\n", desiredHeight_, GetHeightMM());
#endif
        
           /*
#ifdef SIMULATOR
        // Turning gripper on and off for simulator
        disengageGripperAtDest_ = false;
        if (HAL::IsGripperEngaged()) {
          if (  (desiredAngle_ == LIFT_ANGLE_HIGH_LIMIT && desiredHeight_ != LIFT_HEIGHT_CARRY)
              ) {
            //PRINT("WILL DISENGAGE GRIPPER AFTER FINAL LIFT POSITION REACHED\n");
            disengageGripperAtDest_ = true;
          }
        } else {
          if (  (desiredAngle_ == LIFT_ANGLE_LOW_LIMIT && desiredHeight_ != LIFT_HEIGHT_LOWDOCK)
             || (desiredHeight_ == LIFT_HEIGHT_CARRY)
              ) {
            HAL::EngageGripper();
          }
        }
#endif
           */
        
        desiredAngle_ = Height2Rad(desiredHeight_);
        angleError_ = desiredAngle_.ToFloat() - currentAngle_.ToFloat();
        
        f32 startRadSpeed = radSpeed_;
        f32 startRad = currentAngle_.ToFloat();
        if (!inPosition_) {
          startRadSpeed = currDesiredRadVel_;
          startRad = currDesiredAngle_;
        } else {
          angleErrorSum_ = 0.f;
        }
        
        lastLiftMovedTime_us = HAL::GetMicroCounter();
        limitingDetected_ = false;
        limitingExpected_ = false;
        inPosition_ = false;
        calibPending_ = false;
        
        if (FLT_NEAR(angleError_,0.f)) {
          inPosition_ = true;
#if(DEBUG_LIFT_CONTROLLER)
          PRINT("Lift: Already at desired position\n");
#endif          
          return;
        }

        
#if(RECALIBRATE_AT_LIMITS)
        // Adjust approach speed to be a little faster if desired height is at a limit.
        approachSpeedRad_ = (desiredHeight_ == LIFT_HEIGHT_LOWDOCK || desiredHeight_ == LIFT_HEIGHT_CARRY) ? 0.5 : 0.2;
#endif
        
        // Start profile of lift trajectory
        vpg_.StartProfile(startRadSpeed, startRad,
                          maxSpeedRad_, accelRad_,
                          approachSpeedRad_, desiredAngle_.ToFloat(),
                          CONTROL_DT);

#if(DEBUG_LIFT_CONTROLLER)
        PRINT("LIFT VPG: startVel %f, startPos %f, maxVel %f, accel %f, endVel %f, endPos %f\n",
              radSpeed_, currentAngle_.ToFloat(), maxSpeedRad_, accelRad_, approachSpeedRad_, desiredAngle_.ToFloat());
#endif
        
      } // SetDesiredHeight_internal()
      
      
      void SetDesiredHeight(f32 height_mm)
      {
        isNodding_ = false;
        SetDesiredHeight_internal(height_mm);
      }
      
      
      f32 GetDesiredHeight()
      {
        return desiredHeight_;
      }
      
      bool IsInPosition(void) {
        return inPosition_;
      }
      
      Result Update()
      {
#if (!defined(SIMULATOR) && (RECALIBRATE_AT_LIMITS == 0))
        // Update routine for calibration sequence
        CalibrationUpdate();
#endif
        
        PoseAndSpeedFilterUpdate();
        
        LimitDetectUpdate();
        
        // If disabled, do not activate motors
        if(!enable_) {
          return RESULT_OK;
        }
        
#if SIMULATOR
        if (disengageGripperAtDest_ && currentAngle_.ToFloat() < disengageAtAngle_) {
          HAL::DisengageGripper();
          disengageGripperAtDest_ = false;
        }
#endif
        
        if(not inPosition_) {
          
          if (!limitingExpected_) {
            // Get the current desired lift angle
            vpg_.Step(currDesiredRadVel_, currDesiredAngle_);
            
            // Compute position error
            angleError_ = currDesiredAngle_ - currentAngle_.ToFloat();
            

            
            // Open loop value to drive at desired speed
            power_ = currDesiredRadVel_ * SPEED_TO_POWER_OL_GAIN;
            
            // Compute corrective value
            f32 power_corr = (Kp_ * angleError_) + (Ki_ * angleErrorSum_);
            
            // Add base power in the direction of the general desired direction
            //power_ += power_corr + ((power_corr > 0) ? BASE_POWER_UP : BASE_POWER_DOWN);
            power_ += power_corr + ((power_ > 0) ? BASE_POWER_UP : BASE_POWER_DOWN);
            
            // Update angle error sum
            angleErrorSum_ += angleError_;
            angleErrorSum_ = CLIP(angleErrorSum_, -MAX_ERROR_SUM, MAX_ERROR_SUM);
          }
          
#if(RECALIBRATE_AT_LIMITS)
          if (!isCalibrated_) {
            power_ = -LIMIT_APPROACH_POWER;
          }
          if (IsLimiting(&calibAngle_)) {
            power_ = 0.f;
            calibPending_ = true;
            inPosition_ = true;

#if(DEBUG_LIFT_CONTROLLER)
            PRINT(" Lift limit detected. Will reset to angle %f\n", calibAngle_);
#endif
            
          } else if (desiredAngle_.ToFloat() == LIFT_ANGLE_LOW_LIMIT || desiredAngle_.ToFloat() == LIFT_ANGLE_HIGH_LIMIT) {
            // Once desired angle is reached, continue to command a power to drive the lift to the limit.
            // We then rely on limit detection to trigger recalibration.
            if (!limitingExpected_) {
              if (currDesiredAngle_ == LIFT_ANGLE_LOW_LIMIT) {
                //power_ = MIN(-MIN_LIMIT_APPROACH_POWER, power_);
                power_ = -LIMIT_APPROACH_POWER;
                limitingExpected_ = true;
#if(DEBUG_LIFT_CONTROLLER)
                PRINT(" Lift low limit reached. (power %f)\n", power_);
#endif
              } else if (currDesiredAngle_ == LIFT_ANGLE_HIGH_LIMIT) {
                //power_ = MAX(MIN_LIMIT_APPROACH_POWER, power_);
                power_ = LIMIT_APPROACH_POWER;
                limitingExpected_ = true;
#if(DEBUG_LIFT_CONTROLLER)
                PRINT(" Lift high limit reached. (power %f)\n", power_);
#endif
              }
            }
            
          } else
#endif // #if(RECALIBRATE_AT_LOW_HEIGHT)
          
          // If accurately tracking current desired angle...
          if((ABS(angleError_) < ANGLE_TOLERANCE && desiredAngle_ == currDesiredAngle_)
             || ABS(currentAngle_ - desiredAngle_) < ANGLE_TOLERANCE) {
              power_ = 0.f;
              inPosition_ = true;
            /*
#ifdef SIMULATOR
              if (disengageGripperAtDest_) {
                HAL::DisengageGripper();
                disengageGripperAtDest_ = false;
              }
#endif
             */
              #if(DEBUG_LIFT_CONTROLLER)
              PRINT(" LIFT HEIGHT REACHED (%f mm)\n", GetHeightMM());
              #endif
          }
          
          
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
          HAL::MotorSetPower(HAL::MOTOR_LIFT, power_);
          
        } // if not in position
        else if(isNodding_)
        {
          // Note that this is inside else(not inPosition), so we must be
          // inPosition if we get here.
          if (GetLastCommandedHeightMM() == nodHighHeight_) {
            SetDesiredHeight_internal(nodLowHeight_);
          } else if (GetLastCommandedHeightMM() == nodLowHeight_) {
            SetDesiredHeight_internal(nodHighHeight_);
            ++numNodsComplete_;
            if(numNodsDesired_ > 0 && numNodsComplete_ >= numNodsDesired_) {
              StopNodding();
            }
          }
        } // else if isNodding
        
        
        if (IsRelaxed() && calibPending_) {
#if(DEBUG_LIFT_CONTROLLER)
          PRINT("RECALIBRATING LIFT\n");
#endif
          ResetAnglePosition(calibAngle_);
          calibPending_ = false;
        }
      
      
        return RESULT_OK;
      }
      
      void SetGains(const f32 kp, const f32 ki, const f32 maxIntegralError)
      {
        Kp_ = kp;
        Ki_ = ki;
        MAX_ERROR_SUM = maxIntegralError;
      }
      
      void Stop()
      {
        isNodding_ = false;
        SetAngularVelocity(0);
      }
      
      void StartNodding(const f32 lowHeight, const f32 highHeight,
                        const u16 period_ms, const s32 numLoops)
      {
        //preNodHeight_  = GetHeightMM();
        nodLowHeight_  = lowHeight;
        nodHighHeight_ = highHeight;
        
        const f32 dAngle = Height2Rad(highHeight) - Height2Rad(lowHeight);
        const f32 speed_rad_per_sec = (2.f*dAngle*1000.f) / static_cast<f32>(period_ms);
        
        SetSpeedAndAccel(speed_rad_per_sec, 1000.f); // TODO: need sane acceleration value
        
        numNodsDesired_  = numLoops;
        numNodsComplete_ = 0;
        isNodding_ = true;
        
        SetDesiredHeight_internal(nodLowHeight_);
        
      } // StartNodding()
      
      
      void StopNodding()
      {
        //SetDesiredHeight_internal(preNodHeight_);
        isNodding_ = false;
      }
      
      bool IsNodding()
      {
        return isNodding_;
      }
      
    } // namespace LiftController
  } // namespace Cozmo
} // namespace Anki
