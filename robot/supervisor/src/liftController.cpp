#include "liftController.h"
#include "imuFilter.h"
#include "messages.h"
#include "pickAndPlaceController.h"
#include "proxSensors.h"
#include "velocityProfileGenerator.h"

#include "anki/common/constantsAndMacros.h"
#include "coretech/common/shared/radians.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/logging.h"

#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"

#include <math.h>

#define DEBUG_LIFT_CONTROLLER 0

// In order to allow charging even when the processes are
// running, automatically disable motors when the robot is on charger.
// This is a temporary measure to support limitations of current HW.
#define DISABLE_MOTORS_ON_CHARGER 1

namespace Anki {
  namespace Cozmo {
    namespace LiftController {

      // Internal function declarations
      void EnableInternal();
      void DisableInternal(bool autoReEnable = false);

      namespace {

        // How long the lift needs to stop moving for before it is considered to be limited.
        const u32 LIFT_STOP_TIME_MS = 500;

        // Amount of time to allow lift to relax with power == 0, before considering it
        // to have settled enough for recalibration.
        const u32 LIFT_RELAX_TIME_MS = 250;

        const f32 MAX_LIFT_CONSIDERED_STOPPED_RAD_PER_SEC = 0.001f;

        const f32 SPEED_FILTERING_COEFF = 0.9f;

        // Used when calling SetDesiredHeight with just a height:
        const f32 DEFAULT_START_ACCEL_FRAC = 0.25f;
        const f32 DEFAULT_END_ACCEL_FRAC   = 0.25f;

        // Physical limits in radians
        const f32 LIFT_ANGLE_LOW_LIMIT_RAD = ConvertLiftHeightToLiftAngleRad(LIFT_HEIGHT_LOWDOCK);
        const f32 LIFT_ANGLE_HIGH_LIMIT_RAD = ConvertLiftHeightToLiftAngleRad(LIFT_HEIGHT_CARRY);

        // If the lift angle falls outside of the range defined by these thresholds, do not use D control.
        // This is to prevent vibrating that tends to occur at the physical limits.
        const f32 NO_D_ANGULAR_RANGE_RAD = DEG_TO_RAD(5.f);
        const f32 USE_PI_CONTROL_LIFT_ANGLE_LOW_THRESH_RAD = LIFT_ANGLE_LOW_LIMIT_RAD + NO_D_ANGULAR_RANGE_RAD;
        const f32 USE_PI_CONTROL_LIFT_ANGLE_HIGH_THRESH_RAD = LIFT_ANGLE_HIGH_LIMIT_RAD - NO_D_ANGULAR_RANGE_RAD;

#ifdef SIMULATOR
        // Only angles greater than this can contribute to error
        // This is to prevent micro-oscillations in sim which make the lift
        // never actually stop moving
        const f32 ENCODER_ANGLE_RES = DEG_TO_RAD_F32(0.35f);

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
        f32 Kd_ = 3000.f;  // derivative gain
        f32 Ki_ = 0.1f;    // integral control constant
        f32 angleErrorSum_ = 0.f;
        f32 MAX_ERROR_SUM = 5.f;

        // Constant power bias to counter gravity
        const f32 ANTI_GRAVITY_POWER_BIAS = 0.15f;
#endif

        // Amount by which angleErrorSum decays to MAX_ANGLE_ERROR_SUM_IN_POSITION
        const f32 ANGLE_ERROR_SUM_DECAY_STEP = 0.02f;

        // If it exceeds this value, applied power should decay to this value when in position.
        // This value should be slightly less than the motor burnout protection threshold (POWER_THRESHOLD[])
        // in syscon's motors.cpp since the actual applied power can be slightly more than this.
        const f32 MAX_POWER_IN_POSITION = 0.24f;

        // Motor burnout protection
        u32 potentialBurnoutStartTime_ms_ = 0;
        const f32 BURNOUT_POWER_THRESH =  Ki_ * MAX_ERROR_SUM + Kp_ * LIFT_ANGLE_TOL;
        const u32 BURNOUT_TIME_THRESH_MS = 2000.f;

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
        f32 maxSpeedRad_ = M_PI_F;
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
          LCS_SET_CURR_ANGLE,
          LCS_COMPLETE
        } LiftCalibState;

        LiftCalibState calState_ = LCS_IDLE;

        // Whether or not lift is calibrated
        bool isCalibrated_ = false;

        // If this is the first time calibrating, repeat until it's done.
        // Shouldn't proceed until calibration is complete.
        bool firstCalibration_ = true;

        // Last time lift movement was detected
        u32 lastLiftMovedTime_ms = 0;

        // Parameters for determining if lift is being messed with during
        // calibration in which case calibration is aborted
        f32 lowLiftAngleDuringCalib_rad_;
        u32 liftAngleHigherThanCalibAbortAngleCount_;
        const f32 UPWARDS_LIFT_MOTION_FOR_CALIB_ABORT_RAD = DEG_TO_RAD(10.f);
        const u32 UPWARDS_LIFT_MOTION_FOR_CALIB_ABORT_CNT = 5;


        // Whether or not to command anything to motor
        bool enable_ = true;

        // Whether or not motor was enabled via Enable()
        // which is used to determine if it should be automatically
        // re-enabled after it leaves the charger.
        bool enabledExternally_ = false;

        // If disabled, lift motor is automatically re-enabled at this time if non-zero.
        u32 enableAtTime_ms_ = 0;

        // If enableAtTime_ms_ is non-zero, this is the time beyond current time
        // that the motor will be re-enabled if the lift is not moving.
        const u32 REENABLE_TIMEOUT_MS = 2000;

        // Bracing for impact
        // Lowers lift quickly and then disables
        // Prevents any new heights from being commanded
        bool bracing_ = false;

        // Checking for cube on lift by lowering power and seeing if there's lift movement
        bool checkForLoadWhenInPosition_ = false;
        u32  checkingForLoadStartTime_ = 0;
        f32  checkingForLoadStartAngle_ = 0;
        void (*checkForLoadCallback_)(bool) = NULL;
        const u32 CHECKING_FOR_LOAD_TIMEOUT_MS = 500;
        const f32 CHECKING_FOR_LOAD_ANGLE_DIFF_THRESH = DEG_TO_RAD_F32(1.f);

      } // "private" members



      Result Init()
      {
        return RESULT_OK;
      }

      void ResetAnglePosition(f32 currAngle)
      {
        currentAngle_ = currAngle;
        desiredAngle_ = currentAngle_;
        currDesiredAngle_ = currentAngle_.ToFloat();
        desiredHeight_ = GetHeightMM();
      }

      void EnableInternal()
      {
        if (!enable_) {
          enable_ = true;
          enableAtTime_ms_ = 0;  // Reset auto-enable trigger time

          ResetAnglePosition(currentAngle_.ToFloat());
#ifdef SIMULATOR
          // SetDesiredHeight might engage the gripper, but we don't want it engaged right now.
          HAL::DisengageGripper();
#endif
        }
      }

      void Enable()
      {
        enabledExternally_ = true;
        EnableInternal();
      }

      void DisableInternal(bool autoReEnable)
      {
        if (enable_) {
          enable_ = false;

          inPosition_ = true;
          prevAngleError_ = 0.f;
          angleErrorSum_ = 0.f;

          if (!IsCalibrating()) {
            power_ = 0;
            HAL::MotorSetPower(MotorID::MOTOR_LIFT, power_);
          }

          potentialBurnoutStartTime_ms_ = 0;
          bracing_ = false;
        }
        enableAtTime_ms_ = 0;
        if (autoReEnable) {
          enableAtTime_ms_ = HAL::GetTimeStamp() + REENABLE_TIMEOUT_MS;
        }
      }

      void Disable(bool autoReEnable)
      {
        enabledExternally_ = false;
        DisableInternal(autoReEnable);
      }

      void StartCalibrationRoutine(bool autoStarted)
      {
        calState_ = LCS_LOWER_LIFT;
        isCalibrated_ = false;
        potentialBurnoutStartTime_ms_ = 0;
        Messages::SendMotorCalibrationMsg(MotorID::MOTOR_LIFT, true, autoStarted);
        angleErrorSum_ = 0.f;
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
              power_ = HAL::MotorGetCalibPower(MotorID::MOTOR_LIFT);
              HAL::MotorSetPower(MotorID::MOTOR_LIFT, power_);
              lastLiftMovedTime_ms = HAL::GetTimeStamp();
              lowLiftAngleDuringCalib_rad_ = currentAngle_.ToFloat();
              liftAngleHigherThanCalibAbortAngleCount_ = 0;
              calState_ = LCS_WAIT_FOR_STOP;
              break;

            case LCS_WAIT_FOR_STOP:
              // Check for when lift stops moving for 0.2 seconds
              if (!IsMoving()) {

                if (HAL::GetTimeStamp() - lastLiftMovedTime_ms > LIFT_STOP_TIME_MS) {
                  // Turn off motor
                  power_ = 0;  // Not strong enough to lift motor, but just enough to unwind backlash. Not sure if this is actually helping.
                  HAL::MotorSetPower(MotorID::MOTOR_LIFT, power_);

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
                AnkiInfo( "LiftController.Calibrated", "");
                ResetAnglePosition(LIFT_ANGLE_LOW_LIMIT_RAD);

                HAL::MotorResetPosition(MotorID::MOTOR_LIFT);
                prevHalPos_ = HAL::MotorGetPosition(MotorID::MOTOR_LIFT);
                calState_ = LCS_COMPLETE;
                // Intentional fall-through
              } else {
                break;
              }
            case LCS_COMPLETE:
            {
              // Turn off motor
              power_ = 0;
              HAL::MotorSetPower(MotorID::MOTOR_LIFT, power_);

              Messages::SendMotorCalibrationMsg(MotorID::MOTOR_LIFT, false);

              isCalibrated_ = true;
              firstCalibration_ = false;
              calState_ = LCS_IDLE;
              break;
            }

          }  // end switch(calState_)

          // Check if lift is actually moving up when it should be moving down.
          // This means someone's messing with it so just abort calibration.
          if (IsCalibrating()) {
            const float currAngle = currentAngle_.ToFloat();
            if (lowLiftAngleDuringCalib_rad_ > currAngle) {
              lowLiftAngleDuringCalib_rad_ = currAngle;
            }

            if (currAngle - lowLiftAngleDuringCalib_rad_ > UPWARDS_LIFT_MOTION_FOR_CALIB_ABORT_RAD) {
              // Must be beyond threshold for some count to ignore
              // lift bouncing against lower limit
              ++liftAngleHigherThanCalibAbortAngleCount_;
              if (liftAngleHigherThanCalibAbortAngleCount_ >= UPWARDS_LIFT_MOTION_FOR_CALIB_ABORT_CNT) {
                if (firstCalibration_) {
                  AnkiWarn("LiftController.CalibrationUpdate.RestartingCalib",
                           "Someone is probably messing with lift (low: %fdeg, curr: %fdeg)",
                           RAD_TO_DEG(lowLiftAngleDuringCalib_rad_), RAD_TO_DEG(currAngle));
                  calState_ = LCS_LOWER_LIFT;
                } else {
                  AnkiInfo("LiftController.CalibrationUpdate.Abort",
                           "Someone is probably messing with lift (low: %fdeg, curr: %fdeg)",
                           RAD_TO_DEG(lowLiftAngleDuringCalib_rad_), RAD_TO_DEG(currAngle));

                  // Maintain current calibration
                  ResetAnglePosition(currAngle);
                  calState_ = LCS_COMPLETE;
                }
              }
            } else {
              liftAngleHigherThanCalibAbortAngleCount_ = 0;
            }
          }

        }
      }


      f32 GetLastCommandedHeightMM()
      {
        return desiredHeight_;
      }

      f32 GetHeightMM()
      {
        return ConvertLiftAngleToLiftHeightMM(currentAngle_.ToFloat());
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
          targetHeight = GetHeightMM();
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
        f32 measuredSpeed = Cozmo::HAL::MotorGetSpeed(MotorID::MOTOR_LIFT);

        radSpeed_ = (measuredSpeed *
                     (1.0f - SPEED_FILTERING_COEFF) +
                     (radSpeed_ * SPEED_FILTERING_COEFF));

        // Update position
        currentAngle_ += (HAL::MotorGetPosition(MotorID::MOTOR_LIFT) - prevHalPos_);

#if(DEBUG_LIFT_CONTROLLER)
        AnkiDebug( "LiftController", "LIFT FILT: speed %f, speedFilt %f, currentAngle %f, currHalPos %f, prevPos %f, pwr %f\n",
              measuredSpeed, radSpeed_, currentAngle_.ToFloat(), HAL::MotorGetPosition(MotorID::MOTOR_LIFT), prevHalPos_, power_);
#endif
        prevHalPos_ = HAL::MotorGetPosition(MotorID::MOTOR_LIFT);
      }

      void SetDesiredHeight_internal(f32 height_mm, f32 acc_start_frac, f32 acc_end_frac, f32 duration_seconds,
                                     const f32 speed_rad_per_sec,
                                     const f32 accel_rad_per_sec2,
                                     bool useVPG)
      {
#if DISABLE_MOTORS_ON_CHARGER
        // If a lift motion is commanded while the robot is on charger,
        // re-enable the motor as long as it wasn't disabled external to
        // this file (e.g. via EnableMotorPower msg).
        if (HAL::BatteryIsOnCharger() && enabledExternally_) {
          EnableInternal();
        }
#endif

        if (!enable_ || bracing_) {
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
            disengageAtAngle_ = ConvertLiftHeightToLiftAngleRad(newDesiredHeight + 3.f*LIFT_FINGER_HEIGHT);
          }
          else {
            disengageGripperAtDest_ = false;
          }
        }
#endif
        // Check if already at desired height
        if (inPosition_ &&
            (ConvertLiftHeightToLiftAngleRad(newDesiredHeight) == desiredAngle_) &&
            (fabsf((desiredAngle_ - currentAngle_).ToFloat()) < LIFT_ANGLE_TOL) ) {
          #if(DEBUG_LIFT_CONTROLLER)
          AnkiDebug( "LiftController", "Already at desired height %f", newDesiredHeight);
          #endif
          return;
        }

        desiredHeight_ = newDesiredHeight;
        desiredAngle_ = ConvertLiftHeightToLiftAngleRad(desiredHeight_);

        // Convert desired height into the necessary angle:
#if(DEBUG_LIFT_CONTROLLER)
        AnkiDebug( "LiftController", "LIFT DESIRED HEIGHT: %f mm (curr height %f mm), duration = %f s", desiredHeight_, GetHeightMM(), duration_seconds);
#endif


        f32 startRadSpeed = radSpeed_;
        f32 startRad = currDesiredAngle_;
        if (!inPosition_) {
          vpg_.Step(startRadSpeed, startRad);
        } else {
          // If already in position, reset angleErrorSum_.
          // Small and short lift motions can be overpowered by the unwinding of
          // accumulated error and not render well/consistently.
          angleErrorSum_ = 0.f;
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
            AnkiInfo("LiftController.SetDesiredHeight.VPGFixedDurationFailed", "startVel %f, startPos %f, acc_start_frac %f, acc_end_frac %f, endPos %f, duration %f. Trying VPG without fixed duration.",
                     startRadSpeed,
                     startRad,
                     acc_start_frac,
                     acc_end_frac,
                     desiredAngle_.ToFloat(),
                     duration_seconds);
          }
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
        AnkiDebug( "LiftController", "VPG (fixedDuration): startVel %f, startPos %f, acc_start_frac %f, acc_end_frac %f, endPos %f, duration %f\n",
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

        if (fabsf(power_ - ANTI_GRAVITY_POWER_BIAS) < BURNOUT_POWER_THRESH || bracing_) {
          potentialBurnoutStartTime_ms_ = 0;
          return false;
        }

        if (potentialBurnoutStartTime_ms_ == 0) {
          potentialBurnoutStartTime_ms_ = HAL::GetTimeStamp();
        } else if (HAL::GetTimeStamp() - potentialBurnoutStartTime_ms_ > BURNOUT_TIME_THRESH_MS) {
          if (IsInPosition() || IMUFilter::IsPickedUp() || ProxSensors::IsAnyCliffDetected()) {
            // Stop messing with the lift! Going limp until you do!
            AnkiInfo("LiftController.MotorBurnoutProtection.GoingLimp", "");
            Messages::SendMotorAutoEnabledMsg(MotorID::MOTOR_LIFT, false);
            DisableInternal(true);
          } else {
            // Burnout protection triggered. Recalibrating.
            AnkiInfo("LiftController.MotorBurnoutProtection.Recalibrating", "");
            StartCalibrationRoutine(true);
          }
          return true;
        }

        return false;
      }

      void Brace() {
        EnableInternal();
        SetDesiredHeight(LIFT_HEIGHT_LOWDOCK, MAX_LIFT_SPEED_RAD_PER_S, MAX_LIFT_ACCEL_RAD_PER_S2);
        bracing_ = true;
      }

      void Unbrace() {
        bracing_ = false;
        if (enabledExternally_) {
          EnableInternal();
        } else {
          DisableInternal();
        }
      }

      Result Update()
      {
        u32 currTime = HAL::GetTimeStamp();

        // Update routine for calibration sequence
        CalibrationUpdate();

        PoseAndSpeedFilterUpdate();

        if (!IsCalibrated()) {
          return RESULT_OK;
        }

#if DISABLE_MOTORS_ON_CHARGER
        if (inPosition_ && HAL::BatteryIsOnCharger()) {
          // Disables motor if robot placed on charger and it's
          // not currently moving to a target angle.
          DisableInternal();
        } else if (enabledExternally_ && enableAtTime_ms_ == 0) {
          // Otherwise re-enables lift if it wasn't disabled external
          // to this file (e.g. via EnableMotorPower msg) and it's
          // not scheduled to auto-enable because it was originally
          // disabled by motor burnout protection
          EnableInternal();
        }
#endif

        // If disabled, do not activate motors
        if(!enable_) {
          if (enableAtTime_ms_ == 0) {
            return RESULT_OK;
          }

          // Auto-enable check
          if (IsMoving()) {
            enableAtTime_ms_ = currTime + REENABLE_TIMEOUT_MS;
            return RESULT_OK;
          } else if (enabledExternally_ && currTime >= enableAtTime_ms_) {
            Messages::SendMotorAutoEnabledMsg(MotorID::MOTOR_LIFT, true);
            EnableInternal();
          } else {
            return RESULT_OK;
          }
        }

        if (MotorBurnoutProtection()) {
          return RESULT_OK;
        }

        if (bracing_ && IsInPosition()) {
          DisableInternal();
          return RESULT_OK;
        }

#ifdef SIMULATOR
        if (disengageGripperAtDest_ && currentAngle_.ToFloat() < disengageAtAngle_) {
          HAL::DisengageGripper();
          disengageGripperAtDest_ = false;
        }
#endif

        if (checkingForLoadStartTime_ > 0) {
          if (currTime > checkingForLoadStartTime_ + CHECKING_FOR_LOAD_TIMEOUT_MS) {
            AnkiInfo( "LiftController.Update.NoLoadDetected", "");
            checkForLoadWhenInPosition_ = false;
            checkingForLoadStartTime_ = 0;
            if (checkForLoadCallback_) {
              checkForLoadCallback_(false);
            }
          } else if (currentAngle_ < checkingForLoadStartAngle_ - CHECKING_FOR_LOAD_ANGLE_DIFF_THRESH) {
            AnkiInfo( "LiftController.Update.LoadDetected", "in %d ms", currTime - checkingForLoadStartTime_);
            checkForLoadWhenInPosition_ = false;
            checkingForLoadStartTime_ = 0;
            if (checkForLoadCallback_) {
              checkForLoadCallback_(true);
            }
          } else {
            // Make sure motor is unpowered while checking for load
            power_ = 0;
            HAL::MotorSetPower(MotorID::MOTOR_LIFT, power_);
            return RESULT_OK;
          }
        }

        // Get the current desired lift angle
        if (currDesiredAngle_ != desiredAngle_) {
          f32 currDesiredRadVel;
          vpg_.Step(currDesiredRadVel, currDesiredAngle_);
        }

        // Compute position error
        f32 angleError = currDesiredAngle_ - currentAngle_.ToFloat();

#ifdef SIMULATOR
        // Ignore if it's less than encoder resolution
        if (ABS(angleError) < ENCODER_ANGLE_RES) {
          angleError = 0;
        }
#endif

        // Compute power
        const f32 powerP = Kp_ * angleError;
        const f32 powerD = Kd_ * (angleError - prevAngleError_) * CONTROL_DT;
        const f32 powerI = Ki_ * angleErrorSum_;
        power_ = ANTI_GRAVITY_POWER_BIAS + powerP + powerD + powerI;

        // Remove D term if lift is near limits
        if ((currentAngle_.ToFloat() < USE_PI_CONTROL_LIFT_ANGLE_LOW_THRESH_RAD &&
             currDesiredAngle_ < USE_PI_CONTROL_LIFT_ANGLE_LOW_THRESH_RAD) ||
            (currentAngle_.ToFloat() > USE_PI_CONTROL_LIFT_ANGLE_HIGH_THRESH_RAD &&
             currDesiredAngle_ > USE_PI_CONTROL_LIFT_ANGLE_HIGH_THRESH_RAD)) {
          power_ -= powerD;
        }


        // If accurately tracking final desired angle...
        if((ABS(angleError) < LIFT_ANGLE_TOL) && (desiredAngle_ == currDesiredAngle_)) {

          // Decay angleErrorSum as long as power exceeds MAX_POWER_IN_POSITION
          if (ABS(power_) > MAX_POWER_IN_POSITION) {
            const f32 decay = ANGLE_ERROR_SUM_DECAY_STEP * (angleErrorSum_ > 0 ? 1.f : -1.f);
            angleErrorSum_ -= decay;
          } else if (checkForLoadWhenInPosition_ && !IsMoving()) {
            checkingForLoadStartTime_ = currTime;
            checkingForLoadStartAngle_ = currentAngle_.ToFloat();
            AnkiInfo( "LiftController.Update.CheckingForLoad", "%d", checkingForLoadStartTime_);
            power_ = 0;
          }

          if (lastInPositionTime_ms_ == 0) {
            lastInPositionTime_ms_ = currTime;
          } else if (currTime - lastInPositionTime_ms_ > IN_POSITION_TIME_MS) {

            inPosition_ = true;
#if(DEBUG_LIFT_CONTROLLER)
            AnkiDebug( "LiftController", " LIFT HEIGHT REACHED (%f mm)", GetHeightMM());
#endif
          }
        } else {
          // Not near final desired angle yet
          lastInPositionTime_ms_ = 0;

          // Only accumulate integral error when not in position
          angleErrorSum_ += angleError;
        }

        // Clip integral error term
        angleErrorSum_ = CLIP(angleErrorSum_, -MAX_ERROR_SUM, MAX_ERROR_SUM);
        prevAngleError_ = angleError;


#if(DEBUG_LIFT_CONTROLLER)
        AnkiDebugPeriodic(50, "LiftController.Update.Values", "LIFT: currA %f, curDesA %f, currVel %f, desA %f, err %f, errSum %f, inPos %d",
                          currentAngle_.ToFloat(),
                          currDesiredAngle_,
                          radSpeed_,
                          desiredAngle_.ToFloat(),
                          angleError,
                          angleErrorSum_,
                          inPosition_ ? 1 : 0);
        AnkiDebugPeriodic(50, "LiftController.Update.Power", "P: %f, I: %f, D: %f, total: %f",
                          (Kp_ * angleError),
                          (Ki_ * angleErrorSum_),
                          (Kd_ * (angleError - prevAngleError_) * CONTROL_DT),
                          power_);
#endif

        power_ = CLIP(power_, -1.0, 1.0);
        HAL::MotorSetPower(MotorID::MOTOR_LIFT, power_);

        return RESULT_OK;
      }

      void SetGains(const f32 kp, const f32 ki, const f32 kd, const f32 maxIntegralError)
      {
        Kp_ = kp;
        Ki_ = ki;
        Kd_ = kd;
        MAX_ERROR_SUM = maxIntegralError;
        AnkiInfo( "LiftController.SetGains", "New lift gains: kp = %f, ki = %f, kd = %f, maxSum = %f",
                 Kp_, Ki_, Kd_, MAX_ERROR_SUM);
      }

      void Stop()
      {
        SetAngularVelocity(0);
      }

      void SendLiftLoadMessage(bool hasLoad) {
        RobotInterface::LiftLoad msg;
        msg.hasLoad = hasLoad;
        RobotInterface::SendMessage(msg);
      }

      void CheckForLoad(void (*callback)(bool))
      {
#ifdef SIMULATOR
        if (callback) {
          callback(HAL::IsGripperEngaged());
        }
#else
        checkForLoadWhenInPosition_ = true;
        checkingForLoadStartTime_ = 0;
        checkForLoadCallback_ = callback;
#endif
      }

    } // namespace LiftController
  } // namespace Cozmo
} // namespace Anki
