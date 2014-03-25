/**
 * File: steeringController.c
 *
 * Author: Hanns Tappeiner (hanns)
 *
 **/

#include <math.h>

#include "anki/cozmo/robot/cozmoConfig.h"
#include "dockingController.h"
#include "anki/cozmo/robot/localization.h"
#include "anki/cozmo/robot/pathFollower.h"
#include "anki/cozmo/robot/speedController.h"
#include "anki/cozmo/robot/steeringController.h"
#include "anki/cozmo/robot/wheelController.h"
//#include "hal/portable.h"
//#include "hal/encoders.h"
#include "anki/cozmo/robot/trace.h"
#include "anki/cozmo/robot/debug.h"

#include "anki/cozmo/robot/hal.h"

#include "anki/common/robot/trig_fast.h"

#define INVALID_IDEAL_FOLLOW_LINE_IDX s16_MAX

namespace Anki {
  namespace Cozmo {
  namespace SteeringController {

   
    // Private namespace
    namespace {
      //Steering gains: Heading tracking gain K1, Crosstrack approach rate K2
      f32 K1_ = DEFAULT_STEERING_K1;
      f32 K2_ = DEFAULT_STEERING_K2;

      bool isInit_ = false;

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
      f32 angularDecel_;
      
      f32 currAngularVel_;
      bool startedPointTurn_;
      
      // If distance to target is less than this, point turn is considered to be complete.
      const float POINT_TURN_TARGET_DIST_STOP_RAD = 0.05;

      // Maximum rotation speed of robot
      f32 maxRotationWheelSpeedDiff = 0.f;
      
    } // Private namespace
    
    // Private function declarations
    //Non linear version of the steering controller (For SM_PATH_FOLLOW)
    void RunLineFollowControllerNL(f32 location_pix, float headingError_rad);
    void ManagePathFollow();
    void ManagePointTurn();
    void ManageDirectDrive();
    
    
    void ReInit()
    {
      isInit_ = false;
    }
    
    //sets the steering controller constants
    void SetGains(float k1, float k2)
    {
      K1_ = k1;
      K2_ = k2;
    }
    
    
    SteerMode GetMode()
    {
      return currSteerMode_;
    }
    
    //This manages at a high level what the steering controller needs to do (steer, use open loop, etc.)
    void Manage()
    {
#if(DEBUG_STEERING_CONTROLLER)
      PRINT("STEER MODE: %d\n", currSteerMode_);
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
      if (wheelSpeedDiff > 2*WheelController::MAX_WHEEL_SPEED_MM_S) {
        *higherWheelSpeed -= avgSpeed;
        *lowerWheelSpeed -= avgSpeed;
      }
      
      // If higherWheelSpeed exceeds max, decrease both wheel speeds
      if (*higherWheelSpeed > WheelController::MAX_WHEEL_SPEED_MM_S) {
        *lowerWheelSpeed -= *higherWheelSpeed - WheelController::MAX_WHEEL_SPEED_MM_S;
        *higherWheelSpeed -= *higherWheelSpeed - WheelController::MAX_WHEEL_SPEED_MM_S;
      }
      
      // If lowerWheelSpeed is faster than negative max, increase both wheel speeds
      if (*lowerWheelSpeed < -WheelController::MAX_WHEEL_SPEED_MM_S) {
        *higherWheelSpeed -= *lowerWheelSpeed + WheelController::MAX_WHEEL_SPEED_MM_S;
        *lowerWheelSpeed -= *lowerWheelSpeed + WheelController::MAX_WHEEL_SPEED_MM_S;
      }
      
      // TODO: Should we also make sure that neither the left or right wheel is
      // driving at a speed that is less than the minimum wheel speed?
      // What's the point of commanding unreachable desired speeds?
      // ...
      
      
      // Check turning speed limit
      if (maxRotationWheelSpeedDiff > 0) {
        wheelSpeedDiff = *higherWheelSpeed - *lowerWheelSpeed;
        if (wheelSpeedDiff > maxRotationWheelSpeedDiff) {
          f32 speedAdjust = 0.5*(wheelSpeedDiff - maxRotationWheelSpeedDiff);
          *higherWheelSpeed -= speedAdjust;
          *lowerWheelSpeed += speedAdjust;
          PRINT("  Wheel speed adjust: (%f, %f), adjustment %f\n", *higherWheelSpeed, *lowerWheelSpeed, speedAdjust);
        }
      }
      
    }
    
    
    /**
     * @brief     Crosstrack steering controller for skid-steered Anki cars
     * @details   This control law uses the crosstrack error and vehicle speed to determine appropriate
     *            left and right wheel pwm commands to converge on zero crosstrack error. The controller operates by attempting
     *            to turn the vehicle to have a heading w.r.t the path that is the arctan of a gain times the crosstrack error,
     *            normalized by speed.  Due to the normalization by speed, it should converge at a constant rate for a given gain.
     *            The speed_to_pwm constant is known to vary across the full range of speeds - a curve which could be included to
     *            enhance performance.  However, the conversion factor selected was simulated to work over a wide range of speeds
     *            and assumptions about turn centers.
     * @date      July 9th, 2012
     * @version   1.2
     * @author    Gabe Hoffmann (gabe.hoffmann@gmail.com), Hanns Tappeiner
     * @copyright This original code is provided to Anki, Inc under royalty-free non-exclusive license per terms of consulting agreement.
     *            Gabe Hoffmann retains ownership of previous work herein: the control law, clipping logic, and filter.
     * @param location_pix   The crosstrack location of the vehicle, in pixels, uncorrected for sensor center.
     * @param wleft
     * @param wright
     */
    void RunLineFollowControllerNL(f32 location_pix, float headingError_rad)
    {
      
      //We only steer in certain cases, for example if the car is supposed to move
      static bool steering_active = FALSE;
      
      // Measured error in meters
      //float xtrack_error = (location_pix - LIN_ARRAY_CENTER_PIXEL) * LINE_ARRAY_MM_PER_PIX * M_PER_MM; // Crosstrack error (m)
      float xtrack_error = location_pix * M_PER_MM; // Crosstrack error (m)
      
      float xtrack_speed;
      static float xtrack_error_last; // Holds value of previous crosstrack error for next loop iteration
      //static u8 isInit = 0; // (initialization flag, false on first function call only)
      
      // Initization
      if (not isInit_) {
        xtrack_speed = 0.f; // because xtrack_error_last never had a chance to be set
        isInit_ = true;
      } else {
        xtrack_speed = (xtrack_error - xtrack_error_last) / Cozmo::CONTROL_DT;
      }
      xtrack_error_last = xtrack_error;
      
      // Control law
      float curvature = 0;
      
      //Get the current vehicle speed (based on encoder values etc.) in mm/sec
      s16 currspeed = SpeedController::GetCurrentMeasuredVehicleSpeed();
      //Get the desired vehicle speed (the one the user commanded to the car)
      s16 desspeed = SpeedController::GetControllerCommandedVehicleSpeed();
      
      
      ///////////////////////////////////////////////////////////////////////////////
      
      // Activate steering if: We are moving and the commanded speed is bigger than
      // zero (or bigger than 0+eps)
      if(SpeedController::IsVehicleStopped() == FALSE && desspeed > SpeedController::SPEED_CONSIDER_VEHICLE_STOPPED_MM_S) {
        steering_active = TRUE;
        
      }
      
      
      // Desired behavior?  We probably only want the robot to actively steering when it's attempting to follow a path.
      // When it's not following a path, you should be able to push it around freely.
      
      //Deactivate steering if: We are not really moving and the commanded speed is zero (or smaller than 0+eps)
      if (SpeedController::IsVehicleStopped() == TRUE && desspeed <= SpeedController::SPEED_CONSIDER_VEHICLE_STOPPED_MM_S) {
        steering_active = false;
        
        // Set wheel controller coast mode as we finish decelerating to 0
        WheelController::SetCoastMode(true);
      }
      
      // If we're commanding any non-zero speed, don't coast
      if(desspeed > SpeedController::SPEED_CONSIDER_VEHICLE_STOPPED_MM_S) {
        WheelController::SetCoastMode(false);
      }
      
      ///////////////////////////////////////////////////////////////////////////////
      
      if (steering_active == TRUE)
      {
        //convert speed to meters per second
        float speedmps = currspeed * M_PER_MM;
        //float attitude = asin_fast(xtrack_speed / speedmps);
        float attitude = -headingError_rad;
        curvature = -K1_ * (atan_fast(K2_ * xtrack_error / (speedmps + 0.2f)) + attitude);
        //We should allow this to go somewhat negative I think... but not too much
        
        Tracefloat(TRACE_VAR_ATTITUDE, attitude, TRACE_MASK_MOTOR_CONTROLLER);
        Tracefloat(TRACE_VAR_XTRACK_ERROR, xtrack_error, TRACE_MASK_MOTOR_CONTROLLER);
        Tracefloat(TRACE_VAR_XTRACK_SPEED, xtrack_speed, TRACE_MASK_MOTOR_CONTROLLER);
        Tracefloat(TRACE_VAR_SPEEDMPS, speedmps, TRACE_MASK_MOTOR_CONTROLLER);
        Tracefloat(TRACE_VAR_CURVATURE, curvature, TRACE_MASK_MOTOR_CONTROLLER);
        
      } else {

        curvature = 0;
 
      }
      
      
      //Convert the curvature to 1/mm
      curvature *= 1000.0f;
      
      // TODO: Get rid of this??
      //STOP: This will make us coast when we command 0, good for now,
      //but we might need to break later
      if (desspeed < 0) {
        curvature = 0;
      }
      
#if(DEBUG_STEERING_CONTROLLER)
      PRINT(" STEERING: headingErr: %f, headingRad: %f, currSpeed: %d\n", location_pix, headingError_rad, currspeed);
      PRINT(" STEERING: xtrack_err: %f, xtrack_speed: %f, attitude: %f, curvature: %f\n", xtrack_error, xtrack_speed, asin_fast(xtrack_speed / ((f32)currspeed * 0.001f)), curvature);
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
      PRINT(" STEERING: %d (L), %d (R)\n", wleft, wright);
#endif
      
      //Command the desired wheel speeds to the wheels
      WheelController::SetDesiredWheelSpeeds(wleft, wright);
    }
    
    
    void SetPathFollowMode()
    {
      currSteerMode_ = SM_PATH_FOLLOW;
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
            fidx = CLIP(fidx, -4, 4);  // TODO: Loosen this up the closer we get to the block?????
            pathRadErr = CLIP(pathRadErr, -0.2, 0.2);
          }
          
          PERIODIC_PRINT(1000,"fidx: %f, distErr %f, radErr %f\n", fidx, pathDistErr, pathRadErr);
          //PRINT("fidx: %f, distErr %f, radErr %f\n", fidx, pathDistErr, pathRadErr);
#if(DEBUG_MAIN_EXECUTION)
          {
            using namespace Sim::OverlayDisplay;
            SetText(PATH_ERROR, "PathError: %.2f mm, %.1f deg  => fidx: %f",
                    pathDistErr, pathRadErr * (180.f/M_PI),
                    fidx);
          }
#endif
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
      //PRINT("DIRECT DRIVE %f %f\n", left_vel, right_vel);
      currSteerMode_ = SM_DIRECT_DRIVE;
      
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
     
      if (leftAccelPerCycle_ == 0) {
        // max acceleration (i.e. command target velocity)
        currLeftVel = targetLeftVel_;
      } else {
        if (ABS(currLeftVel - targetLeftVel_) < ABS(leftAccelPerCycle_)) {
          currLeftVel = targetLeftVel_;
        } else {
          currLeftVel += leftAccelPerCycle_;
        }
      }
      
      if (rightAccelPerCycle_ == 0) {
        // max acceleration (i.e. command target velocity)
        currRightVel = targetRightVel_;
      } else {
        if (ABS(currRightVel - targetRightVel_) < ABS(rightAccelPerCycle_)) {
          currRightVel = targetRightVel_;
        } else {
          currRightVel += rightAccelPerCycle_;
        }
      }
      
      WheelController::SetDesiredWheelSpeeds(currLeftVel, currRightVel);
      
    }
    
    void ExecutePointTurn(f32 targetAngle, f32 maxAngularVel, f32 angularAccel, f32 angularDecel)
    {
      currSteerMode_ = SM_POINT_TURN;
      
      // Stop the robot if not already
      if (SpeedController::GetUserCommandedDesiredVehicleSpeed() != 0) {
        SpeedController::SetUserCommandedDesiredVehicleSpeed(0);
      }
      
      targetRad_ = targetAngle;
      maxAngularVel_ = maxAngularVel;
      angularAccel_ = angularAccel;
      angularDecel_ = angularDecel;
      startedPointTurn_ = false;
    }
    
    void ManagePointTurn()
    {
      if (!SpeedController::IsVehicleStopped() && !startedPointTurn_) {
        RunLineFollowControllerNL(0,0);
        return;
      }

      startedPointTurn_ = true;
      
      
      // TODO(kevin): Computate max reachable angular velocity and angular distance to target where we begin slowing down.
      //...
      
      
      // Compute distance to target
      Radians currAngle = Cozmo::Localization::GetCurrentMatOrientation();
      float angularDistToTarget = currAngle.angularDistance(targetRad_, maxAngularVel_ < 0);
      
      // TODO(kevin): Update current angular velocity.
      //...For now, just command constant angular speed
      currAngularVel_ = PIDIV2;
      
      // Check for stop condition
      if (ABS(angularDistToTarget) < POINT_TURN_TARGET_DIST_STOP_RAD) {
        currSteerMode_ = SM_PATH_FOLLOW;
        currAngularVel_ = 0;
#if(DEBUG_STEERING_CONTROLLER)
        PRINT("POINT TURN: Stopping\n");
#endif
      }
      
      // Compute the velocity along the arc length equivalent of currAngularVel.
      // currAngularVel_ / PI = arcVel / (PI * R)
      s16 arcVel = (s16)(currAngularVel_ * WHEEL_DIST_HALF_MM); // mm/s
      
      // Compute the wheel velocities
      s16 wleft = -arcVel;
      s16 wright = arcVel;

      
#if(DEBUG_STEERING_CONTROLLER)
      PRINT("POINT TURN: angularDistToTarget: %f radians, arcVel: %d mm/s\n", angularDistToTarget, arcVel);
#endif
      
      WheelController::SetDesiredWheelSpeeds(wleft, wright);
    }
    
    
  } // namespace SteeringController
  } // namespace Cozmo
} // namespace Anki


