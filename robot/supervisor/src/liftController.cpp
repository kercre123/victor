#include "liftController.h"
#include "pickAndPlaceController.h"
#include "imuFilter.h"
#include <math.h>
#include "anki/common/constantsAndMacros.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/logging.h"
#include "messages.h"
#include "radians.h"
#include "velocityProfileGenerator.h"

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
        const f32 ENCODER_ANGLE_RES = DEG_TO_RAD_F32(0.35f);
        
        // Motor burnout protection
        u32 potentialBurnoutStartTime_ms_ = 0;
        const f32 BURNOUT_POWER_THRESH = 0.6;
        const u32 BURNOUT_TIME_THRESH_MS = 2000.f;

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

        // The height of the "fingers"
        const f32 LIFT_FINGER_HEIGHT = 3.8f;
#else
        f32 Kp_ = 3.f;     // proportional control constant
        f32 Kd_ = 2000.f;  // derivative gain
        f32 Ki_ = 0.1f;    // integral control constant
        f32 angleErrorSum_ = 0.f;
        f32 MAX_ERROR_SUM = 5.f;

        // Constant power bias to counter gravity
        const f32 ANTI_GRAVITY_POWER_BIAS = 0.15f;
#endif
        
        // Amount by which angleErrorSum decays to MAX_ANGLE_ERROR_SUM_IN_POSITION
        const f32 ANGLE_ERROR_SUM_DECAY_STEP = 0.02f;
        
        // If it exceeds this value, applied power should decay to this value when in position.
        // This value should match the motor burnout protection threshold (POWER_THRESHOLD[]) in syscon's motors.cpp.
        const f32 MAX_POWER_IN_POSITION = 0.25;
        
        
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


        // Calibration parameters
        typedef enum {
          LCS_IDLE,
          LCS_LOWER_LIFT,
          LCS_WAIT_FOR_STOP,
          LCS_SET_CURR_ANGLE
        } LiftCalibState;

        LiftCalibState calState_ = LCS_IDLE;

        bool isCalibrated_ = true;
        u32 lastLiftMovedTime_ms = 0;


        // Whether or not to command anything to motor
        bool enable_ = false;
        
        // If disabled, lift motor is automatically re-enabled at this time if non-zero.
        u32 enableAtTime_ms_ = 0;
        
        // If enableAtTime_ms_ is non-zero, this is the time beyond current time
        // that the motor will be re-enabled if the lift is not moving.
        const u32 REENABLE_TIMEOUT_MS = 2000;
        
        // Bracing for impact
        // Lowers lift quickly and then disables
        // Prevents any new heights from being commanded
        bool bracing_ = false;

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
          enableAtTime_ms_ = 0;  // Reset auto-enable trigger time

          currDesiredAngle_ = currentAngle_.ToFloat();
          SetDesiredHeight(Rad2Height(currentAngle_.ToFloat()));
        }
      }

      void Disable(bool autoReEnable)
      {
        if (enable_) {
          enable_ = false;

          inPosition_ = true;
          prevAngleError_ = 0.f;
          angleErrorSum_ = 0.f;

          power_ = 0;
          HAL::MotorSetPower(MOTOR_LIFT, power_);
          
          potentialBurnoutStartTime_ms_ = 0;
          bracing_ = false;
          
          if (autoReEnable) {
            enableAtTime_ms_ = HAL::GetTimeStamp() + REENABLE_TIMEOUT_MS;
          }
        }
      }


      void ResetAnglePosition(f32 currAngle)
      {
        currentAngle_ = currAngle;
        desiredAngle_ = currentAngle_;
        currDesiredAngle_ = currentAngle_.ToFloat();
        desiredHeight_ = Rad2Height(currAngle);

        HAL::MotorResetPosition(MOTOR_LIFT);
        prevHalPos_ = HAL::MotorGetPosition(MOTOR_LIFT);
        isCalibrated_ = true;
      }

      void StartCalibrationRoutine(bool autoStarted)
      {
        if (!IsCalibrating()) {
          Enable();
          calState_ = LCS_LOWER_LIFT;
          isCalibrated_ = false;
          potentialBurnoutStartTime_ms_ = 0;
          Messages::SendMotorCalibrationMsg(MOTOR_LIFT, true, autoStarted);
        }
      }

      bool IsCalibrated()
      {
        return isCalibrated_;
      }
      
      bool IsCalibrating()
      {
        return calState_ != LCS_IDLE;
      }
      
      void ClearCalibration()
      {
        isCalibrated_ = false;
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
              HAL::MotorSetPower(MOTOR_LIFT, power_);
              lastLiftMovedTime_ms = HAL::GetTimeStamp();
              calState_ = LCS_WAIT_FOR_STOP;
              break;

            case LCS_WAIT_FOR_STOP:
              // Check for when lift stops moving for 0.2 seconds
              if (!IsMoving()) {

                if (HAL::GetTimeStamp() - lastLiftMovedTime_ms > LIFT_STOP_TIME_MS) {
                  // Turn off motor
                  power_ = 0;  // Not strong enough to lift motor, but just enough to unwind backlash. Not sure if this is actually helping.
                  HAL::MotorSetPower(MOTOR_LIFT, power_);

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
                ResetAnglePosition(LIFT_ANGLE_LOW_LIMIT);
                calState_ = LCS_IDLE;
                Messages::SendMotorCalibrationMsg(MOTOR_LIFT, false);
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

      void SetMaxSpeedAndAccel(const f32 max_speed_rad_per_sec, const f32 accel_rad_per_sec2)
      {
        maxSpeedRad_ = ABS(max_speed_rad_per_sec);
        accelRad_ = ABS(accel_rad_per_sec2);
        
        if (NEAR_ZERO(maxSpeedRad_)) {
          maxSpeedRad_ = MAX_LIFT_SPEED_RAD_PER_S;
        }
        if (NEAR_ZERO(accelRad_)) {
          accelRad_ = MAX_LIFT_ACCEL_RAD_PER_S2;
        }

        maxSpeedRad_ = CLIP(maxSpeedRad_, 0, MAX_LIFT_SPEED_RAD_PER_S);
        accelRad_    = CLIP(accelRad_, 0, MAX_LIFT_ACCEL_RAD_PER_S2);
      }

      void SetAngularVelocity(const f32 speed_rad_per_sec, const f32 accel_rad_per_sec2)
      {
        // Command a target height based on the sign of the desired speed
        bool useVPG = true;
        f32 targetHeight = 0.f;
        if (speed_rad_per_sec > 0.f) {
          targetHeight = LIFT_HEIGHT_CARRY;
        } else if (speed_rad_per_sec < 0.f) {
          targetHeight = LIFT_HEIGHT_LOWDOCK;
        } else {
          // Stop immediately!
          targetHeight = Rad2Height(currentAngle_.ToFloat());
          useVPG = false;
        }

        SetDesiredHeight(targetHeight, speed_rad_per_sec, accel_rad_per_sec2, useVPG);
      }

      f32 GetAngularVelocity()
      {
        return radSpeed_;
      }

      void PoseAndSpeedFilterUpdate()
      {
        // Get encoder speed measurements
        f32 measuredSpeed = Cozmo::HAL::MotorGetSpeed(MOTOR_LIFT);

        radSpeed_ = (measuredSpeed *
                     (1.0f - SPEED_FILTERING_COEFF) +
                     (radSpeed_ * SPEED_FILTERING_COEFF));

        // Update position
        currentAngle_ += (HAL::MotorGetPosition(MOTOR_LIFT) - prevHalPos_);

#if(DEBUG_LIFT_CONTROLLER)
        AnkiDebug( 16, "LiftController", 308, "LIFT FILT: speed %f, speedFilt %f, currentAngle %f, currHalPos %f, prevPos %f, pwr %f\n", 6,
              measuredSpeed, radSpeed_, currentAngle_.ToFloat(), HAL::MotorGetPosition(MOTOR_LIFT), prevHalPos_, power_);
#endif
        prevHalPos_ = HAL::MotorGetPosition(MOTOR_LIFT);
      }

      void SetDesiredHeight_internal(f32 height_mm, f32 acc_start_frac, f32 acc_end_frac, f32 duration_seconds,
                                     const f32 speed_rad_per_sec,
                                     const f32 accel_rad_per_sec2,
                                     bool useVPG)
      {
        if (bracing_) {
          return;
        }
        
        SetMaxSpeedAndAccel(speed_rad_per_sec, accel_rad_per_sec2);

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
        // Check if already at desired height
        if (inPosition_ &&
            (Height2Rad(newDesiredHeight) == desiredAngle_) &&
            (ABS((desiredAngle_ - currentAngle_).ToFloat()) < LIFT_ANGLE_TOL) ) {
          #if(DEBUG_LIFT_CONTROLLER)
          AnkiDebug( 16, "LiftController", 145, "Already at desired height %f", 1, newDesiredHeight);
          #endif
          return;
        }

        desiredHeight_ = newDesiredHeight;
        desiredAngle_ = Height2Rad(desiredHeight_);

        // Convert desired height into the necessary angle:
#if(DEBUG_LIFT_CONTROLLER)
        AnkiDebug( 16, "LiftController", 146, "LIFT DESIRED HEIGHT: %f mm (curr height %f mm), duration = %f s", 3, desiredHeight_, GetHeightMM(), duration_seconds);
#endif


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

          AnkiConditionalWarn(res, 16, "LiftController", 147, "FAIL: VPG (fixedDuration): startVel %f, startPos %f, acc_start_frac %f, acc_end_frac %f, endPos %f, duration %f. Trying VPG without fixed duration.\n", 6,
                  startRadSpeed, startRad, acc_start_frac, acc_end_frac, desiredAngle_.ToFloat(), duration_seconds);
        }
        if (!res) {
          f32 vpgSpeed = maxSpeedRad_;
          f32 vpgAccel = accelRad_;
          if (!useVPG) {
            // If not useVPG, just use really large velocity and accelerations
            vpgSpeed = 1000000.f;
            vpgAccel = 1000000.f;
          }
          
          vpg_.StartProfile(startRadSpeed, startRad,
                            vpgSpeed, vpgAccel,
                            0, desiredAngle_.ToFloat(),
                            CONTROL_DT);
        }

#if DEBUG_LIFT_CONTROLLER
        AnkiDebug( 16, "LiftController", 148, "VPG (fixedDuration): startVel %f, startPos %f, acc_start_frac %f, acc_end_frac %f, endPos %f, duration %f\n", 6,
              startRadSpeed, startRad, acc_start_frac, acc_end_frac, desiredAngle_.ToFloat(), duration_seconds);
#endif
      } // SetDesiredHeight_internal


      void SetDesiredHeightByDuration(f32 height_mm, f32 acc_start_frac, f32 acc_end_frac, f32 duration_seconds)
      {
        SetDesiredHeight_internal(height_mm, acc_start_frac, acc_end_frac, duration_seconds,
                                  MAX_LIFT_SPEED_RAD_PER_S, MAX_LIFT_ACCEL_RAD_PER_S2, true);
      }

      void SetDesiredHeight(f32 height_mm,
                            f32 speed_rad_per_sec,
                            f32 accel_rad_per_sec2,
                            bool useVPG)
      {
        SetDesiredHeight_internal(height_mm, DEFAULT_START_ACCEL_FRAC, DEFAULT_END_ACCEL_FRAC, 0,
                                  speed_rad_per_sec, accel_rad_per_sec2, useVPG);
      }
      
      f32 GetDesiredHeight()
      {
        return desiredHeight_;
      }

      bool IsInPosition(void) {
        return inPosition_;
      }

      // Check for conditions that could lead to motor burnout.
      // If motor is powered at greater than BURNOUT_POWER_THRESH for more than BURNOUT_TIME_THRESH_MS, stop it!
      // If the lift was in position, assuming that someone is messing with the motor.
      // If the lift was not in position, assuming that it's mis-calibrated and it's hitting the low or high hard limit. Do calibration.
      // Returns true if a protection action was triggered.
      bool MotorBurnoutProtection() {
        
        if (ABS(power_) < BURNOUT_POWER_THRESH || bracing_) {
          potentialBurnoutStartTime_ms_ = 0;
          return false;
        }
        
        if (potentialBurnoutStartTime_ms_ == 0) {
          potentialBurnoutStartTime_ms_ = HAL::GetTimeStamp();
        } else if (HAL::GetTimeStamp() - potentialBurnoutStartTime_ms_ > BURNOUT_TIME_THRESH_MS) {
          if (IsInPosition() || IMUFilter::IsPickedUp() || HAL::IsCliffDetected()) {
            // Stop messing with the lift! Going limp until you do!
            Messages::SendMotorAutoEnabledMsg(MOTOR_LIFT, false);
            Disable(true);
          } else {
            // Burnout protection triggered. Recalibrating.
            StartCalibrationRoutine(true);
          }
          return true;
        }
        
        return false;
      }
      
      void Brace() {
        SetDesiredHeight(LIFT_HEIGHT_LOWDOCK, MAX_LIFT_SPEED_RAD_PER_S, MAX_LIFT_ACCEL_RAD_PER_S2);
        bracing_ = true;
      }
      
      void Unbrace() {
        bracing_ = false;
        Enable();
      }
      
      Result Update()
      {
        // Update routine for calibration sequence
        CalibrationUpdate();

        PoseAndSpeedFilterUpdate();

        // If disabled, do not activate motors
        if(!enable_) {
          if (enableAtTime_ms_ == 0) {
            return RESULT_OK;
          }
          
          // Auto-enable check
          if (IsMoving()) {
            enableAtTime_ms_ = HAL::GetTimeStamp() + REENABLE_TIMEOUT_MS;
            return RESULT_OK;
          } else if (HAL::GetTimeStamp() >= enableAtTime_ms_) {
            Messages::SendMotorAutoEnabledMsg(MOTOR_LIFT, true);
            Enable();
          } else {
            return RESULT_OK;
          }
        }

        if (!IsCalibrated()) {
          return RESULT_OK;
        }
        
        if (MotorBurnoutProtection()) {
          return RESULT_OK;
        }

        if (bracing_ && IsInPosition()) {
          Disable();
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
        if((ABS(angleError) < LIFT_ANGLE_TOL) && (desiredAngle_ == currDesiredAngle_)) {

          // Keep angleErrorSum from accumulating once we're in position
          angleErrorSum_ -= angleError;
          
          // Decay angleErrorSum as long as power exceeds MAX_POWER_IN_POSITION
          if (ABS(angleErrorSum_) > MAX_POWER_IN_POSITION) {
            f32 decay = ANGLE_ERROR_SUM_DECAY_STEP * (angleErrorSum_ > 0 ? 1.f : -1.f);
            angleErrorSum_ -= decay;
          }
          
          if (lastInPositionTime_ms_ == 0) {
            lastInPositionTime_ms_ = HAL::GetTimeStamp();
          } else if (HAL::GetTimeStamp() - lastInPositionTime_ms_ > IN_POSITION_TIME_MS) {

            inPosition_ = true;
#if(DEBUG_LIFT_CONTROLLER)
            AnkiDebug( 16, "LiftController", 152, " LIFT HEIGHT REACHED (%f mm)", 1, GetHeightMM());
#endif
          }
        } else {
          lastInPositionTime_ms_ = 0;
        }


#if(DEBUG_LIFT_CONTROLLER)
        AnkiDebugPeriodic(50, 389, "LiftController.Update.Values", 613, "LIFT: currA %f, curDesA %f, currVel %f, desA %f, err %f, errSum %f, inPos %d", 7,
                          currentAngle_.ToFloat(),
                          currDesiredAngle_,
                          radSpeed_,
                          desiredAngle_.ToFloat(),
                          angleError,
                          angleErrorSum_,
                          inPosition_ ? 1 : 0);
        AnkiDebugPeriodic(50, 390, "LiftController.Update.Power", 614, "P: %f, I: %f, D: %f, total: %f", 4,
                          (Kp_ * angleError),
                          (Ki_ * angleErrorSum_),
                          (Kd_ * (angleError - prevAngleError_) * CONTROL_DT),
                          power_);
#endif

        power_ = CLIP(power_, -1.0, 1.0);
        HAL::MotorSetPower(MOTOR_LIFT, power_);

        return RESULT_OK;
      }

      void SetGains(const f32 kp, const f32 ki, const f32 kd, const f32 maxIntegralError)
      {
        Kp_ = kp;
        Ki_ = ki;
        Kd_ = kd;
        MAX_ERROR_SUM = maxIntegralError;
        AnkiInfo( 280, "LiftController.SetGains", 563, "New lift gains: kp = %f, ki = %f, kd = %f, maxSum = %f", 4,
                 Kp_, Ki_, Kd_, MAX_ERROR_SUM);
      }

      void Stop()
      {
        SetAngularVelocity(0);
      }

    } // namespace LiftController
  } // namespace Cozmo
} // namespace Anki
