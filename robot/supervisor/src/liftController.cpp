#include "liftController.h"
#include "pickAndPlaceController.h"
#include "anki/common/robot/config.h"
#include "anki/common/shared/velocityProfileGenerator.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/common/robot/utilities_c.h"
#include "anki/common/shared/radians.h"
#include "anki/common/robot/errorHandling.h"


#define DEBUG_LIFT_CONTROLLER 0


namespace Anki {
  namespace Cozmo {
    namespace LiftController {
      
      
      namespace {
        
        // How long the lift needs to stop moving for before it is considered to be limited.
        const u32 LIFT_STOP_TIME_MS = 500;
        
        // Amount of time to allow lift to relax with power == 0, before considering it
        // to have settled enough for recalibration.
        const u32 LIFT_RELAX_TIME_MS = 250;
        
        const f32 MAX_LIFT_CONSIDERED_STOPPED_RAD_PER_SEC = 0.001;
        
        const f32 SPEED_FILTERING_COEFF = 0.9f;
        
        // Used when calling SetDesiredHeight with just a height:
        const f32 DEFAULT_START_ACCEL_FRAC = 0.25f;
        const f32 DEFAULT_END_ACCEL_FRAC   = 0.25f;
        
        // Only angles greater than this can contribute to error
        // TODO: Find out what this actually is
        const f32 ENCODER_ANGLE_RES = DEG_TO_RAD(0.35f);
        
        // If angle is within this tolerance of the desired angle
        // we are considered to be in position
        const f32 ANGLE_TOLERANCE = DEG_TO_RAD(1.5f);
        
        // Initialized in Init()
        f32 LIFT_ANGLE_LOW_LIMIT;

#ifdef SIMULATOR
        // For disengaging gripper once the lift has reached its final position
        bool disengageGripperAtDest_ = false;
        f32  disengageAtAngle_ = 0.f;
        
        f32 Kp_ = 3.f; // proportional control constant
        f32 Kd_ = 0.f;  // derivative gain
        f32 Ki_ = 0.f; // integral control constant
        f32 angleErrorSum_ = 0.f;
        f32 MAX_ERROR_SUM = 10.f;
        
        // Constant power bias to counter gravity
        const f32 ANTI_GRAVITY_POWER_BIAS = 0.0f;
#else
        f32 Kp_ = 3.f;    // proportional control constant
        f32 Kd_ = 1250.f;  // derivative gain
        f32 Ki_ = 0.1f; //0.05f;   // integral control constant
        f32 angleErrorSum_ = 0.f;
        f32 MAX_ERROR_SUM = 4.f;
        
        // Open loop gain
        // power_open_loop = SPEED_TO_POWER_OL_GAIN * desiredSpeed + BASE_POWER
        const f32 SPEED_TO_POWER_OL_GAIN_UP = 0.0517f;
        const f32 BASE_POWER_UP[NUM_CARRY_STATES] = {0.2312f, 0.3082f, 0.37f}; // 0.37f is a guesstimate
        const f32 SPEED_TO_POWER_OL_GAIN_DOWN = 0.0521f;
        const f32 BASE_POWER_DOWN[NUM_CARRY_STATES] = {0.1389f, 0.05f, 0.f};

        // Constant power bias to counter gravity
        const f32 ANTI_GRAVITY_POWER_BIAS = 0.15f;
#endif
        // Angle of the main lift arm.
        // On the real robot, this is the angle between the lower lift joint on the robot body
        // and the lower lift joint on the forklift assembly.
        Radians currentAngle_ = 0.f;
        Radians desiredAngle_ = 0.f;
        f32 desiredHeight_ = 0.f;
        f32 currDesiredAngle_ = 0.f;
        f32 prevAngleError_ = 0.f;
        f32 prevHalPos_ = 0.f;
        bool inPosition_  = true;
        
        const u32 IN_POSITION_TIME_MS = 100;
        u32 lastInPositionTime_ms_ = 0;
        
        // Speed and acceleration params
        f32 maxSpeedRad_ = PI_F;
        f32 accelRad_ = 1000.f;
        
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
        f32 nodEaseInFraction_  = 0.5f;
        f32 nodEaseOutFraction_ = 0.5f;
        f32 nodHalfPeriod_sec_  = 0.5f;

        // Tapping
        const u8 MAX_NUM_TAPS_REMAINING = 10;
        
        
        // Calibration parameters
        typedef enum {
          LCS_IDLE,
          LCS_LOWER_LIFT,
          LCS_WAIT_FOR_STOP,
          LCS_SET_CURR_ANGLE
        } LiftCalibState;
        
        LiftCalibState calState_ = LCS_IDLE;

        bool isCalibrated_ = false;
        u32 lastLiftMovedTime_ms = 0;

        
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
        return RESULT_OK;
      }
      
      
      void Enable()
      {
        if (!enable_) {
          enable_ = true;
        
          currDesiredAngle_ = currentAngle_.ToFloat();
          SetDesiredHeight(Rad2Height(currentAngle_.ToFloat()));
        }
      }
      
      void Disable()
      {
        if (enable_) {
          enable_ = false;
          
          inPosition_ = true;
          angleErrorSum_ = 0.f;
          
          power_ = 0;
          HAL::MotorSetPower(HAL::MOTOR_LIFT, power_);
        }
      }
      
      
      void ResetAnglePosition(f32 currAngle)
      {
        currentAngle_ = currAngle;
        desiredAngle_ = currentAngle_;
        currDesiredAngle_ = currentAngle_.ToFloat();
        desiredHeight_ = Rad2Height(currAngle);
        
        HAL::MotorResetPosition(HAL::MOTOR_LIFT);
        prevHalPos_ = HAL::MotorGetPosition(HAL::MOTOR_LIFT);
        isCalibrated_ = true;
      }
      
      void StartCalibrationRoutine()
      {
        PRINT("Starting Lift calibration\n");
        calState_ = LCS_LOWER_LIFT;
        isCalibrated_ = false;
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
              lastLiftMovedTime_ms = HAL::GetTimeStamp();
              calState_ = LCS_WAIT_FOR_STOP;
              break;
              
            case LCS_WAIT_FOR_STOP:
              // Check for when lift stops moving for 0.2 seconds
              if (!IsMoving()) {

                if (HAL::GetTimeStamp() - lastLiftMovedTime_ms > LIFT_STOP_TIME_MS) {
                  // Turn off motor
                  power_ = 0;  // Not strong enough to lift motor, but just enough to unwind backlash. Not sure if this is actually helping.
                  HAL::MotorSetPower(HAL::MOTOR_LIFT, power_);
                  
                  // Set timestamp to be used in next state to wait for motor to "relax"
                  lastLiftMovedTime_ms = HAL::GetTimeStamp();
                  
                  // Go to next state
                  calState_ = LCS_SET_CURR_ANGLE;
                }
              } else {
                lastLiftMovedTime_ms = HAL::GetTimeStamp();
              }
              break;
              
            case LCS_SET_CURR_ANGLE:
              // Wait for motor to relax and then set angle
              if (HAL::GetTimeStamp() - lastLiftMovedTime_ms > LIFT_RELAX_TIME_MS) {
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
      
      f32 ComputeOpenLoopPower(f32 desired_speed_rad_per_sec)
      {
        // Open loop value to drive at desired speed
        f32 power = 0;
        
#ifdef SIMULATOR
        power = desired_speed_rad_per_sec * 0.05;
#else
        CarryState_t cs = PickAndPlaceController::GetCarryState();
        if (desired_speed_rad_per_sec > 0) {
          power = desired_speed_rad_per_sec * SPEED_TO_POWER_OL_GAIN_UP + BASE_POWER_UP[cs];
        } else {
          power = desired_speed_rad_per_sec * SPEED_TO_POWER_OL_GAIN_DOWN - BASE_POWER_DOWN[cs];
        }
#endif
        
        return power;
      }
      
      void SetMaxSpeedAndAccel(const f32 max_speed_rad_per_sec, const f32 accel_rad_per_sec2)
      {
        maxSpeedRad_ = ABS(max_speed_rad_per_sec);
        accelRad_ = accel_rad_per_sec2;
      }
      
      void SetMaxLinearSpeedAndAccel(const f32 max_speed_mm_per_sec, const f32 accel_mm_per_sec2)
      {
        maxSpeedRad_ = max_speed_mm_per_sec / LIFT_ARM_LENGTH;
        accelRad_    = accel_mm_per_sec2 / LIFT_ARM_LENGTH;
      }
      
      void GetMaxSpeedAndAccel(f32 &max_speed_rad_per_sec, f32 &accel_rad_per_sec2)
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
        //power_ = ComputeOpenLoopPower(rad_per_sec);
        //HAL::MotorSetPower(HAL::MOTOR_LIFT, power_);
        
        // Command a target height based on the sign of the desired speed
        f32 targetHeight = 0;
        if (rad_per_sec > 0) {
          targetHeight = LIFT_HEIGHT_CARRY;
          maxSpeedRad_ = rad_per_sec;
        } else if (rad_per_sec < 0) {
          targetHeight = LIFT_HEIGHT_LOWDOCK;
          maxSpeedRad_ = rad_per_sec;
        } else {
          // Compute the expected height if we were to start slowing down now
          f32 radToStop = 0.5f*(radSpeed_*radSpeed_) / accelRad_;
          if (radSpeed_ < 0) {
            radToStop *= -1;
          }
          targetHeight = CLIP(Rad2Height( currentAngle_.ToFloat() + radToStop ), LIFT_HEIGHT_LOWDOCK, LIFT_HEIGHT_CARRY);
          //PRINT("Stopping: radSpeed %f, accelRad %f, radToStop %f, currentAngle %f, targetHeight %f\n",
          //      radSpeed_, accelRad_, radToStop, currentAngle_.ToFloat(), targetHeight);
        }
        SetDesiredHeight(targetHeight);
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
        //PRINT("LiftHeight: %fmm, speed %f, accel %f\n", height_mm, maxSpeedRad_, accelRad_);
        SetDesiredHeight(height_mm, DEFAULT_START_ACCEL_FRAC, DEFAULT_END_ACCEL_FRAC, 0);
      }

      static void SetDesiredHeight_internal(f32 height_mm, f32 acc_start_frac, f32 acc_end_frac, f32 duration_seconds)
      {
        
        // Do range check on height
        const f32 newDesiredHeight = CLIP(height_mm, LIFT_HEIGHT_LOWDOCK, LIFT_HEIGHT_CARRY);
        
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
        PRINT("LIFT DESIRED HEIGHT: %f mm (curr height %f mm), duration = %f s\n", desiredHeight_, GetHeightMM(), duration_seconds);
#endif
        
        
        desiredAngle_ = Height2Rad(desiredHeight_);
        prevAngleError_ = 0;
        
        f32 startRadSpeed = radSpeed_;
        f32 startRad = currDesiredAngle_;
        if (!inPosition_) {
          vpg_.Step(startRadSpeed, startRad);
        }
        
        lastInPositionTime_ms_ = 0;
        inPosition_ = false;
        

        bool res = false;
        if (duration_seconds > 0) {
          res = vpg_.StartProfile_fixedDuration(startRad, startRadSpeed, acc_start_frac*duration_seconds,
                                                   desiredAngle_.ToFloat(), acc_end_frac*duration_seconds,
                                                   MAX_LIFT_SPEED_RAD_PER_S,
                                                   MAX_LIFT_ACCEL_RAD_PER_S2,
                                                   duration_seconds,
                                                   CONTROL_DT);
        
          if (!res) {
            PRINT("FAIL: LIFT VPG (fixedDuration): startVel %f, startPos %f, acc_start_frac %f, acc_end_frac %f, endPos %f, duration %f. Trying VPG without fixed duration.\n",
                  startRadSpeed, startRad, acc_start_frac, acc_end_frac, desiredAngle_.ToFloat(), duration_seconds);
          }
        }
        if (!res) {
          vpg_.StartProfile(startRadSpeed, startRad,
                            maxSpeedRad_, accelRad_,
                            0, desiredAngle_.ToFloat(),
                            CONTROL_DT);
        }
        
#if DEBUG_LIFT_CONTROLLER
        PRINT("LIFT VPG (fixedDuration): startVel %f, startPos %f, acc_start_frac %f, acc_end_frac %f, endPos %f, duration %f\n",
              startRadSpeed, startRad, acc_start_frac, acc_end_frac, desiredAngle_.ToFloat(), duration_seconds);
#endif
      } // SetDesiredHeight_internal

      
      void SetDesiredHeight(f32 height_mm, f32 acc_start_frac, f32 acc_end_frac, f32 duration_seconds)
      {
        isNodding_ = false;
        SetDesiredHeight_internal(height_mm, acc_start_frac, acc_end_frac, duration_seconds);
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
        // Update routine for calibration sequence
        CalibrationUpdate();
        
        PoseAndSpeedFilterUpdate();
        
        // If disabled, do not activate motors
        if(!enable_) {
          return RESULT_OK;
        }
        
        if (!IsCalibrated()) {
          return RESULT_OK;
        }
        
        
#if SIMULATOR
        if (disengageGripperAtDest_ && currentAngle_.ToFloat() < disengageAtAngle_) {
          HAL::DisengageGripper();
          disengageGripperAtDest_ = false;
        }
#endif
        
        

        // Get the current desired lift angle
        if (currDesiredAngle_ != desiredAngle_) {
          f32 currDesiredRadVel;
          vpg_.Step(currDesiredRadVel, currDesiredAngle_);
        }
      
        // Compute position error
        // Ignore if it's less than encoder resolution
        f32 angleError = currDesiredAngle_ - currentAngle_.ToFloat();
        if (ABS(angleError) < ENCODER_ANGLE_RES) {
          angleError = 0;
        }

        
        // Compute power
        power_ = ANTI_GRAVITY_POWER_BIAS + (Kp_ * angleError) + (Kd_ * (angleError - prevAngleError_) * CONTROL_DT) + (Ki_ * angleErrorSum_);
        
        // Update error terms
        prevAngleError_ = angleError;
        angleErrorSum_ += angleError;
        angleErrorSum_ = CLIP(angleErrorSum_, -MAX_ERROR_SUM, MAX_ERROR_SUM);

        

        // If accurately tracking current desired angle...
        if((ABS(angleError) < ANGLE_TOLERANCE) && (desiredAngle_ == currDesiredAngle_)) {
          
          if (lastInPositionTime_ms_ == 0) {
            lastInPositionTime_ms_ = HAL::GetTimeStamp();
          } else if (HAL::GetTimeStamp() - lastInPositionTime_ms_ > IN_POSITION_TIME_MS) {
            
            // Keep angleErrorSum from accumulating once we're in position
            angleErrorSum_ -= angleError;
            
            inPosition_ = true;
#if(DEBUG_LIFT_CONTROLLER)
            PRINT(" LIFT HEIGHT REACHED (%f mm)\n", GetHeightMM());
#endif
          }
        } else {
          lastInPositionTime_ms_ = 0;
        }

        
#if(DEBUG_LIFT_CONTROLLER)
        PERIODIC_PRINT(100, "LIFT: currA %f, curDesA %f, currVel %f, desA %f, err %f, errSum %f, inPos %d, pwr %f\n",
                       currentAngle_.ToFloat(),
                       currDesiredAngle_,
                       radSpeed_,
                       desiredAngle_.ToFloat(),
                       angleError,
                       angleErrorSum_,
                       inPosition_ ? 1 : 0,
                       power_);
        PERIODIC_PRINT(100, "  POWER terms: %f  %f\n", (Kp_ * angleError_), (Ki_ * angleErrorSum_))
#endif
        
        power_ = CLIP(power_, -1.0, 1.0);
        HAL::MotorSetPower(HAL::MOTOR_LIFT, power_);

        

        if(isNodding_ && inPosition_)
        {
          if (GetLastCommandedHeightMM() == nodHighHeight_) {
            angleErrorSum_ = 0;
            SetDesiredHeight_internal(nodLowHeight_, nodEaseOutFraction_, nodEaseInFraction_, nodHalfPeriod_sec_);
          } else if (GetLastCommandedHeightMM() == nodLowHeight_) {
            angleErrorSum_ = 0;
            SetDesiredHeight_internal(nodHighHeight_, nodEaseOutFraction_, nodEaseInFraction_, nodHalfPeriod_sec_);
            ++numNodsComplete_;
            if(numNodsDesired_ > 0 && numNodsComplete_ >= numNodsDesired_) {
              StopNodding();
            }
          }
        }
        
        return RESULT_OK;
      }
      
      void SetGains(const f32 kp, const f32 ki, const f32 kd, const f32 maxIntegralError)
      {
        Kp_ = kp;
        Ki_ = ki;
        Kd_ = kd;
        MAX_ERROR_SUM = maxIntegralError;
        PRINT("New lift gains: kp = %f, ki = %f, kd = %f, maxSum = %f\n",
              Kp_, Ki_, Kd_, MAX_ERROR_SUM);
      }
      
      void Stop()
      {
        isNodding_ = false;
        SetAngularVelocity(0);
      }
      
      void StartNodding(const f32 lowHeight, const f32 highHeight,
                        const u16 period_ms, const s32 numLoops,
                        const f32 easeInFraction, const f32 easeOutFraction)
      {
        AnkiConditionalWarnAndReturn(enable_, "LiftController.StartNodding.Disabled",
                                     "StartNodding() command ignored: LiftController is disabled.\n");
        
        //preNodHeight_  = GetHeightMM();
        nodLowHeight_  = lowHeight;
        nodHighHeight_ = highHeight;
        numNodsDesired_  = numLoops;
        numNodsComplete_ = 0;
        isNodding_ = true;
        nodEaseInFraction_ = easeInFraction;
        nodEaseOutFraction_ = easeOutFraction;
        nodHalfPeriod_sec_ = static_cast<f32>(period_ms) * 0.5f * 0.001f;
        
        SetMaxSpeedAndAccel(10, 20);
        SetDesiredHeight_internal(nodLowHeight_, nodEaseOutFraction_, nodEaseInFraction_, nodHalfPeriod_sec_);
        
      } // StartNodding()
      
      
      void StopNodding()
      {
        AnkiConditionalWarnAndReturn(enable_, "LiftController.StopNodding.Disabled",
                                     "StopNodding() command ignored: LiftController is disabled.\n");
        
        //SetDesiredHeight_internal(preNodHeight_);
        isNodding_ = false;
      }
      
      bool IsNodding()
      {
        return isNodding_;
      }
      
      void TapBlockOnGround(u8 numTaps)
      {
        //PRINT("RECVD TapBlockOnGround %d\n", numTaps);
       
        if (isNodding_) {
          // If already nodding, just increase the number of nods to do
          numNodsDesired_ += numTaps;
          if (numNodsDesired_ > MAX_NUM_TAPS_REMAINING) {
            numNodsDesired_ = MAX_NUM_TAPS_REMAINING;
          }
        } else {
          // Add tap if the lift is already in a low position
          if (GetDesiredHeight() <= LIFT_HEIGHT_LOWDOCK + 5) {
            ++numTaps;
          }
          
#ifdef SIMULATOR
          StartNodding(LIFT_HEIGHT_LOWDOCK, LIFT_HEIGHT_LOWDOCK + 30,
                       300, numTaps, 0, 0.5);
#else
          StartNodding(LIFT_HEIGHT_LOWDOCK+13, LIFT_HEIGHT_LOWDOCK + 25,
                       300, numTaps, 0, 0.5);
#endif
        }
      }
      
      void StopTapping()
      {
        
      }
      
    } // namespace LiftController
  } // namespace Cozmo
} // namespace Anki
