/**
 * File: steeringController.c
 *
 * Author: Hanns Tappeiner (hanns)
 *
 **/

#include <math.h>

#include "anki/cozmo/shared/cozmoConfig.h"
#include "pathFollower.h"
#include "speedController.h"
#include "steeringController.h"
#include "wheelController.h"
#include "imuFilter.h"
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/hal.h"
#include "messages.h"

#include "velocityProfileGenerator.h"
#include "trig_fast.h"

#include "localization.h"
#include "dockingController.h"

#define DEBUG_STEERING_CONTROLLER 0

#define s16_MIN std::numeric_limits<s16>::min()
#define s16_MAX std::numeric_limits<s16>::max()

#define INVALID_IDEAL_FOLLOW_LINE_IDX std::numeric_limits<s16>::max()

namespace Anki {
  namespace Vector {
  namespace SteeringController {


    // Private namespace
    namespace {


      //The gains for the steering controller
      //Heading tracking gain K1, Crosstrack approach rate K2
      const f32 DEFAULT_STEERING_K1 = 0.1f;
      const f32 DEFAULT_STEERING_K2 = 12.f;

      //Steering gains: Heading tracking gain K1, Crosstrack approach rate K2
      f32 K1_ = DEFAULT_STEERING_K1;
      f32 K2_ = DEFAULT_STEERING_K2;

      // Clipping of path offset fed into path follow controller.
      // Used during docking only.
      f32 PATH_DIST_OFFSET_CAP_MM = 20;
      f32 PATH_ANGULAR_OFFSET_CAP_RAD = 0.5;

      // Whether or not the steeringController should be actively
      // trying to drive wheels.
      bool enable_ = true;

      SteerMode currSteerMode_ = SM_PATH_FOLLOW;

      // Direct drive
      f32 targetLeftVel_;
      f32 targetRightVel_;
      f32 leftAccelPerCycle_;
      f32 rightAccelPerCycle_;

      // Point turn
      Radians targetRad_;
      f32 maxAngularVel_;
      f32 angularAccel_;
      bool isPointTurnWithTarget_;

      Radians prevAngle_;
      f32 angularDistExpected_;
      f32 angularDistTraversed_;
      
      // Amount by which theoretical wheel velocities are scaled
      // to compensate for tread slip.
#ifdef SIMULATOR
      const f32 POINT_TURN_SLIP_COMP_FACTOR = 1.f;
#else
      const f32 POINT_TURN_SLIP_COMP_FACTOR = 1.5f;
#endif
      
      f32 pointTurnAngTol_;
      
      f32 pointTurnSpeedKp_ = 20;
      f32 pointTurnSpeedKd_ = 0;
      f32 pointTurnSpeedKi_ = 0.5;
      f32 pointTurnSpeedMaxIntegralError_ = 100;
      
      f32 pointTurnKp_ = 450.f;
#ifdef SIMULATOR
      f32 pointTurnKd_ = 500.f; // Too high a derivative gain causes issues in sim
#else
      f32 pointTurnKd_ = 4000.f;
#endif
      f32 pointTurnKi_ = 20.f;
      f32 pointTurnMaxIntegralError_ = 5;
      f32 prevPointTurnAngleError_ = 0;
      f32 pointTurnAngleErrorSum_ = 0;
      TimeStamp_t inPositionStartTime_ = 0;
      
      // Amount of time orientation is within tolerance of target angle before
      // the controller is considered to have reached the target point turn angle.
      const u32 IN_POSITION_THRESHOLD_MS = 500;
      
      // For checking if robot is not turning despite maxed integral power being applied
      TimeStamp_t pointTurnIntegralPowerMaxedStartTime_ = 0;
      Radians pointTurnIntegralPowerMaxedStartAngle_ = 0;
      const u32 POINT_TURN_STUCK_THRESHOLD_MS = 500;
      const f32 POINT_TURN_STUCK_THRESHOLD_RAD = DEG_TO_RAD_F32(0.5f);
      
      // If angular distance to target is less than this value, zero the P and I terms, and clear error sum
      // (to combat stiction). This effectively limits the accuracy of the turn to this amount (keep it small)
      const f32 POINT_TURN_CONTROLLER_PI_DEADBAND_RAD = DEG_TO_RAD_F32(0.5f);
      
      // Integrator only accumulates if the angular distance to target is within this value, and is zeroed if not
      const f32 POINT_TURN_INTEGRATOR_THRESH_RAD = DEG_TO_RAD_F32(5.f);
      
      // Amount by which to reduce P and D gains for low speed turns, to prevent controller 'stuttering'
      // The thresholds below determine when this is active. (this should be between 0 and 1)
      const f32 POINT_TURN_LOW_SPEED_GAIN_REDUCTION_FACTOR = 0.5f;
      
      // For turns that remain below this speed the entire time (or if the commanded velocity
      // is below this) the P and D gains are reduced to avoid controller stuttering
      const f32 POINT_TURN_REDUCE_GAINS_SPEED_THRESH_RAD_PER_S = DEG_TO_RAD_F32(40.f);
      
      // For turns with a commanded accel below this threshold, reduce the P and D gains if the
      // commanded velocity is below a threshold (to prevent controller stuttering at low speeds)
      const f32 POINT_TURN_REDUCE_GAINS_ACCEL_THRESH_RAD_PER_S2 = DEG_TO_RAD_F32(100.f);
      
      // Maximum rotation speed of robot
      f32 maxRotationWheelSpeedDiff = 0.f;

      VelocityProfileGenerator vpg_;

      const f32 POINT_TURN_TERMINAL_VEL_RAD_PER_S = 0;
      
      f32 recordedHeading_ = 0.f;

    } // Private namespace

    // Private function declarations
    //Non linear version of the steering controller (For SM_PATH_FOLLOW)
    void RunLineFollowControllerNL(f32 offsetError_mm, float headingError_rad);
    void ManagePathFollow();
    void ManagePointTurn();
    void ManageDirectDrive();


    void Enable()
    {
      if (!enable_) {
        WheelController::Enable();
        enable_ = true;
      }
    }

    void Disable()
    {
      if (enable_) {
        ExecuteDirectDrive(0,0);
        WheelController::Disable();
        enable_ = false;
      }
    }


    //sets the steering controller constants
    void SetGains(f32 k1, f32 k2, f32 dockPathDistOffsetCap_mm, f32 dockPathAngularOffsetCap_rad)
    {
      AnkiInfo( "SteeringController.SetGains", "New gains: k1 %f, k2 %f, dist_cap %f mm, ang_cap %f rad",
            k1, k2, dockPathDistOffsetCap_mm, dockPathAngularOffsetCap_rad);
      K1_ = k1;
      K2_ = k2;
      PATH_DIST_OFFSET_CAP_MM = dockPathDistOffsetCap_mm;
      PATH_ANGULAR_OFFSET_CAP_RAD = dockPathAngularOffsetCap_rad;
    }
    
    f32 GetK1Gain()
    {
      return K1_;
    }
    
    f32 GetK2Gain()
    {
      return K2_;
    }
    
    f32 GetPathDistOffsetCap()
    {
      return PATH_DIST_OFFSET_CAP_MM;
    }
    
    f32 GetPathAngOffsetCap()
    {
      return PATH_ANGULAR_OFFSET_CAP_RAD;
    }

    void SetPointTurnGains(f32 kp, f32 ki, f32 kd, f32 maxIntegralError)
    {
      pointTurnKp_ = kp;
      pointTurnKd_ = kd;
      pointTurnKi_ = ki;
      pointTurnMaxIntegralError_ = maxIntegralError;
      /*
      pointTurnSpeedKp_ = kp;
      pointTurnSpeedKd_ = kd;
      pointTurnSpeedKi_ = ki;
      pointTurnSpeedMaxIntegralError_ = maxIntegralError;
      */
      AnkiInfo( "SteeringController.SetPointTurnGains", "New gains: kp %f, ki %f, kd %f, maxSum %f", kp, ki, kd, maxIntegralError);
    }
    
    SteerMode GetMode()
    {
      return currSteerMode_;
    }

    //This manages at a high level what the steering controller needs to do (steer, use open loop, etc.)
    void Manage()
    {
      if (!enable_) {
        return;
      }

#if(DEBUG_STEERING_CONTROLLER)
      AnkiDebug( "SteeringController.Manage.Mode", "%d", currSteerMode_);
#endif
      switch(currSteerMode_) {

        case SM_PATH_FOLLOW:
          ManagePathFollow();
          break;
        case SM_DIRECT_DRIVE:
          ManageDirectDrive();
          break;
        case SM_POINT_TURN:
          ManagePointTurn();
          break;
        default:
          break;

      }

    }


    void SetRotationSpeedLimit(f32 rad_per_s)
    {
      maxRotationWheelSpeedDiff = rad_per_s * WHEEL_DIST_MM;
    }
    
    f32 GetRotationSpeedLimit()
    {
      return maxRotationWheelSpeedDiff;
    }

    void DisableRotationSpeedLimit()
    {
      maxRotationWheelSpeedDiff = 0.f;
    }


    // 1) If either wheel is going faster than possible, shift both speeds down by an offset to preserve curvature.
    // 2) If wheel speeds will cause robot to turn faster than permitted, shift both wheel speed towards each other.
    void CheckWheelSpeedLimits(f32& lSpeed, f32& rSpeed)
    {

      // Do any of the speeds exceed max?
      // If so, then shift both speeds to be within range by the same amount
      // so that desired curvature is still achieved.
      // If we're in path following mode, maintaining proper curvature
      // is probably more important than maintaining speed.
      f32 *lowerWheelSpeed = &lSpeed;
      f32 *higherWheelSpeed = &rSpeed;
      if (lSpeed > rSpeed) {
        lowerWheelSpeed = &rSpeed;
        higherWheelSpeed = &lSpeed;
      }

      f32 wheelSpeedDiff = *higherWheelSpeed - *lowerWheelSpeed;
      f32 avgSpeed = (*higherWheelSpeed + *lowerWheelSpeed) * 0.5f;

      // Center speeds on 0 if wheelSpeedDiff exceeds maximum achievable wheel speed
      if (wheelSpeedDiff > 2*MAX_WHEEL_SPEED_MMPS) {
        *higherWheelSpeed -= avgSpeed;
        *lowerWheelSpeed -= avgSpeed;
      }

      // If higherWheelSpeed exceeds max, decrease both wheel speeds
      if (*higherWheelSpeed > MAX_WHEEL_SPEED_MMPS) {
        *lowerWheelSpeed -= *higherWheelSpeed - MAX_WHEEL_SPEED_MMPS;
        *higherWheelSpeed -= *higherWheelSpeed - MAX_WHEEL_SPEED_MMPS;
      }

      // If lowerWheelSpeed is faster than negative max, increase both wheel speeds
      if (*lowerWheelSpeed < -MAX_WHEEL_SPEED_MMPS) {
        *higherWheelSpeed -= *lowerWheelSpeed + MAX_WHEEL_SPEED_MMPS;
        *lowerWheelSpeed -= *lowerWheelSpeed + MAX_WHEEL_SPEED_MMPS;
      }

      // TODO: Should we also make sure that neither the left or right wheel is
      // driving at a speed that is less than the minimum wheel speed?
      // What's the point of commanding unreachable desired speeds?
      // ...


      // Check turning speed limit
      if (maxRotationWheelSpeedDiff > 0) {
        wheelSpeedDiff = *higherWheelSpeed - *lowerWheelSpeed;
        if (wheelSpeedDiff > maxRotationWheelSpeedDiff) {
          f32 speedAdjust = 0.5f*(wheelSpeedDiff - maxRotationWheelSpeedDiff);
          *higherWheelSpeed -= speedAdjust;
          *lowerWheelSpeed += speedAdjust;
//          AnkiInfo( "SteeringController", "  Wheel speed adjust: (%f, %f), adjustment %f", *higherWheelSpeed, *lowerWheelSpeed, speedAdjust);
        }
      }

    }


    /**
     * @brief     Crosstrack steering controller. Modification of original controller provided by Gabe Hoffmann for Drive.
     * @details   This control law uses the crosstrack error, heading error, and vehicle speed to determine appropriate
     *            left and right wheel pwm commands to converge on zero crosstrack error and zero heading error. The controller operates by attempting
     *            to turn the vehicle to have a heading w.r.t the path that is the arctan of a gain times the crosstrack error,
     *            normalized by speed.  Due to the normalization by speed, it should converge at a constant rate for a given gain.
     *
     * @author    Gabe Hoffmann (gabe.hoffmann@gmail.com), Hanns Tappeiner, Kevin Yoon
     * @copyright This original code is provided to Anki, Inc under royalty-free non-exclusive license per terms of consulting agreement.
     *            Gabe Hoffmann retains ownership of previous work herein: the control law, clipping logic, and filter.
     * @param offsetError_mm    The crosstrack location (or shortest distance to the currently followed path segment) of the robot
     * @param headingError_rad  The angular error between the robot's current orientation and the tangent of the closest point on the current path.
     */
    void RunLineFollowControllerNL(f32 offsetError_mm, float headingError_rad)
    {

      //We only steer in certain cases, for example if the car is supposed to move
      static bool steering_active = FALSE;

      // Control law
      float curvature = 0;

      //Get the current vehicle speed (based on encoder values etc.) in mm/sec
      s16 currspeed = SpeedController::GetCurrentMeasuredVehicleSpeed();
      //Get the controller desired speed for the current tic. This speed should approach user desired speed.
      s16 desspeed = SpeedController::GetControllerCommandedVehicleSpeed();
      //Get the user desired speed
      s16 userDesiredSpeed = SpeedController::GetUserCommandedDesiredVehicleSpeed();


      // If moving backwards, modify distance and angular error such that proper curvature
      // is computed below.
      if (desspeed < 0) {
        offsetError_mm *= -1;
        if (userDesiredSpeed == 0) {
          // If slowing to a stop, then assume zero error
          offsetError_mm = 0;
          headingError_rad = 0;
        } else {
          headingError_rad = -Radians(headingError_rad + M_PI_F).ToFloat();
        }
      }

      ///////////////////////////////////////////////////////////////////////////////

      // Activate steering if: We are moving and the commanded speed is bigger than
      // zero (or bigger than 0+eps)
      if(WheelController::AreWheelsMoving() && ABS(desspeed) > SpeedController::SPEED_CONSIDER_VEHICLE_STOPPED_MM_S) {
        steering_active = TRUE;
      }
      
      // Disable steering while we are changing direction (going from forwards to backwards or backwards to
      // forwards)
      if((currspeed > 0 && userDesiredSpeed < 0) || (currspeed < 0 && userDesiredSpeed > 0))
      {
        steering_active = FALSE;
      }


      // Desired behavior?  We probably only want the robot to actively steering when it's attempting to follow a path.
      // When it's not following a path, you should be able to push it around freely.

      //Deactivate steering if: We are not really moving and the commanded speed is zero (or smaller than 0+eps)
      if (!WheelController::AreWheelsMoving() && ABS(desspeed) <= SpeedController::SPEED_CONSIDER_VEHICLE_STOPPED_MM_S) {
        steering_active = false;

        // Set wheel controller coast mode as we finish decelerating to 0
        WheelController::SetCoastMode(true);
      }

      // If we're commanding any non-zero speed, don't coast
      if(ABS(desspeed) > SpeedController::SPEED_CONSIDER_VEHICLE_STOPPED_MM_S) {
        WheelController::SetCoastMode(false);
      }

      ///////////////////////////////////////////////////////////////////////////////
      if (steering_active == TRUE)
      {
        curvature = -K1_ * (atan_fast(K2_ * offsetError_mm / (ABS(currspeed) + 200)) - headingError_rad);
      } else {
        curvature = 0;
      }

#if(DEBUG_STEERING_CONTROLLER)
      AnkiDebug( "SteeringController.RunLineFollowControllerNL.Errors", "offsetError_mm: %f, headingError_rad: %f, curvature: %f, currSpeed: %d", offsetError_mm, headingError_rad, curvature, currspeed);
#endif


      //We are moving along a circle, so let's compute the speed for the single wheels
      //Let's interpret the delta_speed as a curvature:
      //Curvature is 1/radius
      // Commanded speeds to wheels are desired speed + offsets for curvature

      //if delta speed is positive, the left wheel is supposed to turn slower, it becomes the INNER wheel
      float leftspeed =  (float)desspeed - WHEEL_DIST_HALF_MM * curvature * desspeed;

      //if delta speed is positive, the right wheel is supposed to turn faster, it becomes the OUTER wheel
      float rightspeed = (float)desspeed + WHEEL_DIST_HALF_MM * curvature * desspeed;


      CheckWheelSpeedLimits(leftspeed, rightspeed);

      s16 wleft = (s16)CLIP(leftspeed,s16_MIN,s16_MAX);
      s16 wright = (s16)CLIP(rightspeed,s16_MIN,s16_MAX);

#if(DEBUG_STEERING_CONTROLLER)
      AnkiDebug( "SteeringController.RunLineFollowControllerNL.Speeds", "%d (L), %d (R)", wleft, wright);
#endif

      //Command the desired wheel speeds to the wheels
      WheelController::SetDesiredWheelSpeeds(wleft, wright);
    }


    void SetPathFollowMode()
    {
      currSteerMode_ = SM_PATH_FOLLOW;
    }


    void SetSteerMode(SteerMode mode)
    {
      if (mode != SM_PATH_FOLLOW) {
        PathFollower::ClearPath();
      }
      currSteerMode_ = mode;
    }

    void ManagePathFollow()
    {
      f32 pathDistErr = 0, pathRadErr = 0;
      f32 fidx = INVALID_IDEAL_FOLLOW_LINE_IDX;
      if (PathFollower::IsTraversingPath()) {
        bool gotError = PathFollower::GetPathError(pathDistErr, pathRadErr);

        if (gotError) {
          fidx = pathDistErr;

          // HACK!
          //SetGains(DEFAULT_STEERING_K1, DEFAULT_STEERING_K2);
          if (DockingController::IsBusy()) {
            //SetGains(DEFAULT_STEERING_K1, 5.f);
            fidx = CLIP(fidx, -PATH_DIST_OFFSET_CAP_MM, PATH_DIST_OFFSET_CAP_MM);  // TODO: Loosen this up the closer we get to the block?????
            pathRadErr = CLIP(pathRadErr, -PATH_ANGULAR_OFFSET_CAP_RAD, PATH_ANGULAR_OFFSET_CAP_RAD);
          }

          //PERIODIC_PRINT(1000,"fidx: %f, distErr %f, radErr %f\n", fidx, pathDistErr, pathRadErr);
          //PRINT("fidx: %f, distErr %f, radErr %f\n", fidx, pathDistErr, pathRadErr);

        } else {
          SpeedController::SetUserCommandedDesiredVehicleSpeed(0);
        }
      }

      //If we found a valid followline, let's run the controller
      if (fidx != INVALID_IDEAL_FOLLOW_LINE_IDX) {
        // Run controller and pass in current speed
        RunLineFollowControllerNL( fidx, pathRadErr );

      } else {
        // No steering intention -- unless desired speed is 0
        // we'll continue to use the previously commanded fidx
        if (SpeedController::GetUserCommandedDesiredVehicleSpeed() == 0) {
          RunLineFollowControllerNL( 0, 0);
        }
      }
    }


    void ExecuteDirectDrive(f32 left_vel, f32 right_vel, f32 left_accel, f32 right_accel)
    {
      if (!enable_) {
        AnkiDebug("SteeringController.ExecuteDirectDrive.Disabled", "lspeed: %f mm/s, rspeed: %f mm/s", left_vel, right_vel);
        return;
      }

      //AnkiDebug( "DIRECT DRIVE", "%d: %f   %f  (%f %f)", HAL::GetTimeStamp(), left_vel, right_vel, left_accel, right_accel);
      SetSteerMode(SM_DIRECT_DRIVE);

      // Get current desired wheel speeds
      f32 currLeftVel, currRightVel;
      WheelController::GetDesiredWheelSpeeds(currLeftVel, currRightVel);

      targetLeftVel_ = left_vel;
      targetRightVel_ = right_vel;
      leftAccelPerCycle_ = ABS(left_accel) * CONTROL_DT;
      rightAccelPerCycle_ = ABS(right_accel) * CONTROL_DT;

      if (currLeftVel > targetLeftVel_)
        leftAccelPerCycle_ *= -1;
      if (currRightVel > targetRightVel_)
        rightAccelPerCycle_ *= -1;

    }

    void ManageDirectDrive()
    {
      // Get current desired wheel speeds
      f32 currLeftVel, currRightVel;
      WheelController::GetDesiredWheelSpeeds(currLeftVel, currRightVel);

//      PRINT("CURR: %f %f\n", targetLeftVel_, targetRightVel_);

      if (NEAR_ZERO(leftAccelPerCycle_)) {
        // max acceleration (i.e. command target velocity)
        currLeftVel = targetLeftVel_;
      } else {
        if (fabsf(currLeftVel - targetLeftVel_) < ABS(leftAccelPerCycle_)) {
          currLeftVel = targetLeftVel_;
        } else {
          currLeftVel += leftAccelPerCycle_;
        }
      }

      if (NEAR_ZERO(rightAccelPerCycle_)) {
        // max acceleration (i.e. command target velocity)
        currRightVel = targetRightVel_;
      } else {
        if (fabsf(currRightVel - targetRightVel_) < ABS(rightAccelPerCycle_)) {
          currRightVel = targetRightVel_;
        } else {
          currRightVel += rightAccelPerCycle_;
        }
      }

      WheelController::SetDesiredWheelSpeeds(currLeftVel, currRightVel);

    }


    void ExecutePointTurn_Internal(f32 maxAngularVel, f32 angularAccel)
    {
      currSteerMode_ = SM_POINT_TURN;

      // Stop the robot if not already
      if (SpeedController::GetUserCommandedDesiredVehicleSpeed() != 0) {
        SpeedController::SetUserCommandedDeceleration(10000);
        SpeedController::SetUserCommandedDesiredVehicleSpeed(0);
      }

      maxAngularVel_ = maxAngularVel;
      angularAccel_ = NEAR_ZERO(angularAccel) ? MAX_BODY_ROTATION_ACCEL_RAD_PER_SEC2 : angularAccel;
      prevPointTurnAngleError_ = 0;
      pointTurnAngleErrorSum_ = 0;

      // Check that the maxAngularVel is greater than the terminal speed
      // If not, make it at least that big.
      if (ABS(maxAngularVel_) < POINT_TURN_TERMINAL_VEL_RAD_PER_S) {
        maxAngularVel_ = POINT_TURN_TERMINAL_VEL_RAD_PER_S * (maxAngularVel_ > 0 ? 1 : -1);
        AnkiWarn( "SteeringController.ExecutePointTurn_Internal.PointTurnTooSlow", "Speeding up commanded point turn of %f rad/s to %f rad/s",  maxAngularVel, maxAngularVel_);
      }


    }

    void ExecutePointTurn(f32 angularVel, f32 angularAccel)
    {
      if (!enable_) {
        AnkiDebug("SteeringController.ExecutePointTurn.Disabled", "Speed: %f rad/s", angularVel);
        return;
      }

      AnkiDebug( "SteeringController.ExecutePointTurn_2.Params", "%d: vel %f, accel %f",
                HAL::GetTimeStamp(), RAD_TO_DEG_F32(angularVel), RAD_TO_DEG_F32(angularAccel));
      
      // Check for nan params
      if (isnan(angularVel) || isnan(angularAccel)) {
        AnkiWarn( "SteeringController.ExecutePointTurn_2.NanParamFound", "%f, %f", angularVel, angularAccel);
        return;
      }
      
      if (fabsf(angularVel) > MAX_BODY_ROTATION_SPEED_RAD_PER_SEC) {
        AnkiWarn( "SteeringController.ExecutePointTurn_2.PointTurnTooFast", "Speed of %f deg/s exceeds limit of %f deg/s. Clamping.",
              RAD_TO_DEG_F32(angularVel), MAX_BODY_ROTATION_SPEED_DEG_PER_SEC);
        angularVel = copysignf(MAX_BODY_ROTATION_SPEED_RAD_PER_SEC, angularVel);
      }
      
      isPointTurnWithTarget_ = false;
      ExecutePointTurn_Internal(angularVel, angularAccel);

      prevAngle_ = Localization::GetCurrPose_angle();
      f32 currAngle = prevAngle_.ToFloat();

      // Generate velocity profile
      vpg_.StartProfile(IMUFilter::GetRotationSpeed(),
                        currAngle,
                        maxAngularVel_,
                        angularAccel_,
                        CONTROL_DT);
    }

    void ExitPointTurn()
    {
      AnkiDebug( "SteeringController.ExitPointTurn", "");
      
      WheelController::SetDesiredWheelSpeeds(0,0);
      SetSteerMode(SM_PATH_FOLLOW);
    }

    void ExecutePointTurn(f32 targetAngle, f32 maxAngularVel, f32 angularAccel, f32 angularDecel, f32 angleTolerance, bool useShortestDir, uint16_t numHalfRevolutions)
    {
      if (!enable_) {
        AnkiDebug("SteeringController.ExecutePointTurn.Disabled", "targetAngle: %f rad, Speed: %f rad/s", targetAngle, maxAngularVel);
        return;
      }

      AnkiDebug( "SteeringController.ExecutePointTurn.Params", "%d: target %f, vel %f, accel %f, decel %f, tol %f, shortestDir %d, numHalfRevs %d",
                HAL::GetTimeStamp(),
                RAD_TO_DEG_F32(targetAngle),
                RAD_TO_DEG_F32(maxAngularVel),
                RAD_TO_DEG_F32(angularAccel),
                RAD_TO_DEG_F32(angularDecel),
                RAD_TO_DEG_F32(angleTolerance),
                useShortestDir,
                numHalfRevolutions);
      
      // Check for nan params
      if (isnan(targetAngle)   ||
          isnan(maxAngularVel) ||
          isnan(angularAccel)  ||
          isnan(angularDecel)  ||
          isnan(angleTolerance)) {
        AnkiWarn( "SteeringController.ExecutePointTurn.NanParamFound", "%f, %f, %f, %f, %f",
                 targetAngle, maxAngularVel, angularAccel, angularDecel, angleTolerance);
        return;
      }
      
      if (fabsf(maxAngularVel) > MAX_BODY_ROTATION_SPEED_RAD_PER_SEC) {
        AnkiWarn( "SteeringController.ExecutePointTurn.PointTurnTooFast", "Speed of %f deg/s exceeds limit of %f deg/s. Clamping.",
              RAD_TO_DEG_F32(maxAngularVel), MAX_BODY_ROTATION_SPEED_DEG_PER_SEC);
        maxAngularVel = copysignf(MAX_BODY_ROTATION_SPEED_RAD_PER_SEC, maxAngularVel);
      }
      
      isPointTurnWithTarget_ = true;
      ExecutePointTurn_Internal(maxAngularVel, angularAccel);

      targetRad_ = targetAngle;
      pointTurnAngTol_ = fabsf(angleTolerance);
      prevAngle_ = Localization::GetCurrPose_angle();
      f32 currAngle = prevAngle_.ToFloat();
      f32 destAngle = targetRad_.ToFloat();

      angularDistTraversed_ = 0;

      if (useShortestDir) {
        angularDistExpected_ = (targetAngle - Localization::GetCurrPose_angle()).ToFloat();
        // Update destAngle and maxAngularVel_ so that the shortest turn is executed to reach the goal
        maxAngularVel_ = copysignf(maxAngularVel_, angularDistExpected_);
        destAngle = currAngle + angularDistExpected_;
      } else {
        // Compute target angle that is on the appropriate side of currAngle given the maxAngularVel
        // which determines the turning direction.
        if (currAngle > destAngle && maxAngularVel_ > 0) {
          destAngle += 2*M_PI_F;
        } else if (currAngle < destAngle && maxAngularVel_ < 0) {
          destAngle -= 2*M_PI_F;
        }
        
        // Ensure that the expected angular turn distance is consistent with
        //  the numHalfRevolutions supplied by the engine:
        while (fabsf(destAngle - currAngle) < M_PI_F * (numHalfRevolutions - 0.5f) ) {
          destAngle += copysignf(2*M_PI_F, maxAngularVel);
        }

        while (fabsf(destAngle - currAngle) > M_PI_F * (numHalfRevolutions + 1.5f) ) {
          destAngle -= copysignf(2*M_PI_F, maxAngularVel);
        }
        
        angularDistExpected_ = destAngle - currAngle;
      }
      
      // Exit early if we're already within tolerance of the target and not currently moving.
      // (If wheels are moving we might be outside of tolerance by the time they stop.)
      if ((fabsf(angularDistExpected_) < pointTurnAngTol_) && !WheelController::AreWheelsMoving()) {
        ExitPointTurn();
        AnkiDebug( "SteeringController.ExecutePointTurn.AlreadyAtDest", "");
        return;
      }

      // Generate velocity profile
      vpg_.StartProfile(IMUFilter::GetRotationSpeed(),
                        currAngle,
                        maxAngularVel_,
                        angularAccel_,
                        maxAngularVel_ > 0 ? POINT_TURN_TERMINAL_VEL_RAD_PER_S : -POINT_TURN_TERMINAL_VEL_RAD_PER_S,
                        destAngle,
                        CONTROL_DT);
    }

    // Position-controlled point turn update
    void ManagePointTurn()
    {

      // Update current angular velocity
      f32 currDesiredAngularVel, currDesiredAngle;
      vpg_.Step(currDesiredAngularVel, currDesiredAngle);

      const Radians& currAngle = Vector::Localization::GetCurrPose_angle();
      
      // Compute the velocity along the arc length equivalent of currAngularVel.
      // currDesiredAngularVel / PI = arcVel / (PI * R)
      s16 arcVel = (s16)(currDesiredAngularVel * WHEEL_DIST_HALF_MM * POINT_TURN_SLIP_COMP_FACTOR); // mm/s
      
      
      if (isPointTurnWithTarget_) {

        angularDistTraversed_ += (currAngle - prevAngle_).ToFloat();
        prevAngle_ = currAngle;

        
        // Compute distance to target
        float angularDistToTarget = 0.f;
        
        if (fabsf(angularDistExpected_ - angularDistTraversed_) > M_PI_F) {
          // we still have at least 180 deg left in the turn, so don't compare angles yet
          angularDistToTarget = angularDistExpected_ - angularDistTraversed_;
        } else {
          angularDistToTarget = (targetRad_ - currAngle).ToFloat();
        }
        
        
        // Check for stop condition
        f32 absAngularDistToTarget = fabsf(angularDistToTarget);
        if (absAngularDistToTarget < pointTurnAngTol_) {
          if (inPositionStartTime_ == 0) {
            AnkiDebug( "SteeringController.ManagePointTurn.InRange", "distToTarget %f, currAngle %f, currDesired %f (currTime %d, inPosTime %d)", RAD_TO_DEG_F32(angularDistToTarget), currAngle.getDegrees(), RAD_TO_DEG_F32(currDesiredAngle), HAL::GetTimeStamp(), inPositionStartTime_);
            inPositionStartTime_ = HAL::GetTimeStamp();
          } else if (HAL::GetTimeStamp() - inPositionStartTime_ > IN_POSITION_THRESHOLD_MS || !WheelController::AreWheelsMoving()) {
#           if(DEBUG_STEERING_CONTROLLER)
            f32 lWheelSpeed, rWheelSpeed, lDesSpeed, rDesSpeed;
            WheelController::GetFilteredWheelSpeeds(lWheelSpeed, rWheelSpeed);
            WheelController::GetDesiredWheelSpeeds(lDesSpeed, rDesSpeed);
            AnkiDebug( "SteeringController.ManagePointTurn.Stopping", "currAngle %f, currDesired %f, currVel %f, distTraversed %f, distExpected %f,  wheelSpeeds %f %f, desSpeeds %f %f",
                      currAngle.getDegrees(), RAD_TO_DEG_F32(currDesiredAngle), RAD_TO_DEG_F32(currDesiredAngularVel), RAD_TO_DEG_F32(angularDistTraversed_), RAD_TO_DEG_F32(angularDistExpected_), 0,0,0,0);
#           endif
            ExitPointTurn();
            return;
          }
        } else {
          AnkiDebugPeriodic(100, "SteeringController.ManagePointTurn.OOR", "distToTarget %f, currAngle %f, currDesired %f (currTime %d, inPosTime %d)", RAD_TO_DEG_F32(angularDistToTarget), currAngle.getDegrees(), RAD_TO_DEG_F32(currDesiredAngle), HAL::GetTimeStamp(), inPositionStartTime_);
          inPositionStartTime_ = 0;

          
          // Check if robot has stopped turning despite integral gain maxing out.
          // Something might be physically obstructing the robot from turning.
          if (fabsf(pointTurnAngleErrorSum_) == pointTurnMaxIntegralError_) {
            if (pointTurnIntegralPowerMaxedStartTime_ == 0 || fabsf((pointTurnIntegralPowerMaxedStartAngle_ - currAngle).ToFloat()) < POINT_TURN_STUCK_THRESHOLD_RAD) {
              pointTurnIntegralPowerMaxedStartTime_ = HAL::GetTimeStamp();
              pointTurnIntegralPowerMaxedStartAngle_ = currAngle;
            } else if (HAL::GetTimeStamp() - pointTurnIntegralPowerMaxedStartTime_ > POINT_TURN_STUCK_THRESHOLD_MS) {
              AnkiInfo( "SteeringController.ManagePointTurn.StoppingCuzStuck", "currAngle %f, currDesired %f", currAngle.getDegrees(), RAD_TO_DEG_F32(currDesiredAngle));
              ExitPointTurn();
              return;
            }
          }
          
        }
        
        // PID control for maintaining desired orientation
        const f32 ff = (f32) arcVel;
        
        const f32 angularDistToCurrDesiredAngle = (Radians(currDesiredAngle) - currAngle).ToFloat();
        f32 p = angularDistToCurrDesiredAngle * pointTurnKp_;
        f32 i = pointTurnAngleErrorSum_ * pointTurnKi_;
        f32 d = (angularDistToCurrDesiredAngle - prevPointTurnAngleError_) * pointTurnKd_;
        
        // Stiction correction - zero P and I terms if close to set point (prevents limit cycle due to stiction)
        // See Section 3.2 of http://www.wescottdesign.com/articles/Friction/friction.pdf
        if (absAngularDistToTarget < POINT_TURN_CONTROLLER_PI_DEADBAND_RAD) {
          p = i = 0.f;
          pointTurnAngleErrorSum_ = 0.f;
        }
        
        // Low speed 'stuttering' correction
        // At low speeds, scale back the P and D terms to prevent controller 'stuttering'. Only apply this to either turns
        // that remain at low speed the entire time, or if the accel is low enough for stuttering to be noticeable.
        if ((fabsf(vpg_.GetMaxReachableVel()) < POINT_TURN_REDUCE_GAINS_SPEED_THRESH_RAD_PER_S) ||
              (fabsf(angularAccel_) < POINT_TURN_REDUCE_GAINS_ACCEL_THRESH_RAD_PER_S2 &&
               fabsf(currDesiredAngularVel) < POINT_TURN_REDUCE_GAINS_SPEED_THRESH_RAD_PER_S)) {
          p *= POINT_TURN_LOW_SPEED_GAIN_REDUCTION_FACTOR;
          d *= POINT_TURN_LOW_SPEED_GAIN_REDUCTION_FACTOR;
        }
        
        arcVel = (s16)(ff + p + i + d);
        prevPointTurnAngleError_ = angularDistToCurrDesiredAngle;
        
        // Integral windup protection
        // Only accumulate integral error if we're close to the target angle
        if (absAngularDistToTarget < POINT_TURN_INTEGRATOR_THRESH_RAD) {
          pointTurnAngleErrorSum_ = pointTurnAngleErrorSum_ + angularDistToCurrDesiredAngle;
          pointTurnAngleErrorSum_ = CLIP(pointTurnAngleErrorSum_, -pointTurnMaxIntegralError_, pointTurnMaxIntegralError_);
        } else {
          pointTurnAngleErrorSum_ = 0.f;
        }
        
//        AnkiDebug( "SteeringController.ManagePointTurn.Controller", "timestamp %d, currAngle %.1f, desAngle %.1f, currSpeed %.1f, desSpeed %.1f, arcVel %d, ff %.1f, p %.1f, i %.1f, d %.1f, errSum %.1f",
//                  HAL::GetTimeStamp(),
//                  currAngle.getDegrees(),
//                  RAD_TO_DEG_F32(currDesiredAngle),
//                  RAD_TO_DEG_F32(IMUFilter::GetRotationSpeed()),
//                  RAD_TO_DEG_F32(currDesiredAngularVel),
//                  arcVel, ff, p, i, d,
//                  pointTurnAngleErrorSum_);
      } else {
        
        // PID control for maintaining desired speed
        f32 currAngularSpeed = IMUFilter::GetRotationSpeed();
        f32 angularSpeedError = currDesiredAngularVel - currAngularSpeed;
        arcVel += (s16)(angularSpeedError * pointTurnSpeedKp_
               + (angularSpeedError - prevPointTurnAngleError_) * pointTurnSpeedKd_
               + pointTurnAngleErrorSum_ * pointTurnSpeedKi_);
        
        prevPointTurnAngleError_ = angularSpeedError;
        
        // Only accumulate integral error if desired wheel speed is below max
        if (ABS(arcVel) < MAX_WHEEL_SPEED_MMPS) {
          pointTurnAngleErrorSum_ = pointTurnAngleErrorSum_ + angularSpeedError;
          pointTurnAngleErrorSum_ = CLIP(pointTurnAngleErrorSum_, -pointTurnSpeedMaxIntegralError_, pointTurnSpeedMaxIntegralError_);
        }
        
        //AnkiDebug( "PointTurnSpeed", "des %f deg/s, meas: %f deg/s, arcVel %d mm/s, errorSum %f", RAD_TO_DEG_F32(currDesiredAngularVel), RAD_TO_DEG_F32(currAngularSpeed), arcVel, pointTurnAngleErrorSum_ );
      }


#if(DEBUG_STEERING_CONTROLLER)
      AnkiDebug( "SteeringController.ManagePointTurn.DistToTarget", "angularDistToTarget: %f radians, arcVel: %d mm/s",  angularDistToTarget, arcVel);
#endif

      WheelController::SetDesiredWheelSpeeds(-arcVel, arcVel);
      //WheelController::ResetIntegralGainSums();

      // If target velocity is zero then we might need to stop
      if (!isPointTurnWithTarget_) {
        if (arcVel == 0 && maxAngularVel_ == 0) {
          ExitPointTurn();
          AnkiDebug( "SteeringController.ManagePointTurn.StoppingBecause0Vel", "");
          return;
        }
      }
    }
    
    void ExecuteDriveCurvature(f32 speed, f32 curvatureRadius_mm, f32 accel)
    {
      if (!enable_) {
        AnkiDebug("SteeringController.ExecuteDriveCurvature.Disabled", "Speed: %f mm/s, curvature: %f mm", speed, curvatureRadius_mm);
        return;
      }

      // Only care about magnitude of accel
      accel = ABS(accel);
      
      f32 leftSpeed = 0.f, rightSpeed = 0.f;
      f32 leftAccel = accel, rightAccel = accel;

      // Compute current commanded average wheel speed
      f32 desL, desR;
      WheelController::GetDesiredWheelSpeeds(desL, desR);
      f32 avgWheelSpeed = 0.5f * (desL + desR);
      
      if(curvatureRadius_mm == s16_MAX || curvatureRadius_mm == s16_MIN) {
        // Drive straight
        leftSpeed  = speed;
        rightSpeed = speed;
        
        // Immediately set current desired wheel speeds to match each other
        // so that we accelerate to the target speed in a straight line
        WheelController::SetDesiredWheelSpeeds(avgWheelSpeed, avgWheelSpeed);
        
      } else if(NEAR_ZERO(curvatureRadius_mm)) {
        ExecutePointTurn(speed, accel);
        return;
      } else {
        // Drive an arc
        
        //if speed is positive, the left wheel should turn slower, so
        // it becomes the INNER wheel
        leftSpeed = speed * (1.0f - WHEEL_DIST_HALF_MM / curvatureRadius_mm);
        
        //if speed is positive, the right wheel should turn faster, so
        // it becomes the OUTER wheel
        rightSpeed = speed * (1.0f + WHEEL_DIST_HALF_MM / curvatureRadius_mm);

        // If acceleration is non-zero (i.e. not instantaneous)
        // compute what it should be for each wheel.
        if (!NEAR_ZERO(accel)) {
          // Immediately set current desired wheel speeds to match the same ratio of
          // target wheel speeds so that the specified curvature is maintained
          f32 leftSpeed_instant = avgWheelSpeed * (1.0f - WHEEL_DIST_HALF_MM / curvatureRadius_mm);
          f32 rightSpeed_instant = avgWheelSpeed * (1.0f + WHEEL_DIST_HALF_MM / curvatureRadius_mm);
          WheelController::SetDesiredWheelSpeeds(leftSpeed_instant, rightSpeed_instant);
          
          if (FLT_NEAR(speed, avgWheelSpeed)) {
            leftAccel = 0.f;
            rightAccel = 0.f;
          } else {
            // Compute what the necessary accelerations are for the individual wheels
            // In the amount of time it should take to accelerate from avgWheelSpeed to speed_mmps
            // at a rate of accel, it should take the same amount of time for
            // leftSpeed_instant to accelerate to leftSpeed at a rate of leftAccel, and
            // rightSpeed_instant to accelerate to rightSpeed at a rate of rightAccel
            // e.g. (speed_mmps - avgWheelSpeed) / accel == (leftSpeed - leftSpeed_instant) / leftAccel
            const f32 oneOverSpeedDiff = 1.f / (speed - avgWheelSpeed);
            leftAccel = accel * (leftSpeed - leftSpeed_instant) * oneOverSpeedDiff;
            rightAccel = accel * (rightSpeed - rightSpeed_instant) * oneOverSpeedDiff;
          }
        }
      }
      
      ExecuteDirectDrive(leftSpeed, rightSpeed, leftAccel, rightAccel);
    }
    
    void RecordHeading()
    {
      recordedHeading_ = Localization::GetCurrPose_angle().ToFloat();
    }
    
    // Execute point turn to the heading recorded by RecordHeading() + offset_rad
    void ExecutePointTurnToRecordedHeading(f32 offset_rad,
                                           f32 maxAngularVel_radPerSec,
                                           f32 angularAccel_radPerSec2,
                                           f32 angularDecel_radPerSec2,
                                           f32 angleTolerance_rad,
                                           uint16_t numHalfRevolutions,
                                           bool useShortestDir)
    {
      ExecutePointTurn(recordedHeading_ + offset_rad, maxAngularVel_radPerSec, angularAccel_radPerSec2, angularDecel_radPerSec2, angleTolerance_rad, useShortestDir, numHalfRevolutions);
    }

    
  } // namespace SteeringController
  } // namespace Vector
} // namespace Anki
