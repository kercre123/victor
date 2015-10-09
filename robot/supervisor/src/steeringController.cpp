/**
 * File: steeringController.c
 *
 * Author: Hanns Tappeiner (hanns)
 *
 **/

#include <math.h>

#include "anki/cozmo/shared/cozmoConfig.h"
#include "dockingController.h"
#include "localization.h"
#include "pathFollower.h"
#include "speedController.h"
#include "steeringController.h"
#include "wheelController.h"
#include "imuFilter.h"
#include "anki/cozmo/robot/hal.h"
#include "messages.h"

#include "anki/common/shared/velocityProfileGenerator.h"
#include "anki/common/robot/trig_fast.h"

#define DEBUG_STEERING_CONTROLLER 0

#define INVALID_IDEAL_FOLLOW_LINE_IDX s16_MAX

namespace Anki {
  namespace Cozmo {
  namespace SteeringController {

   
    // Private namespace
    namespace {
      //Steering gains: Heading tracking gain K1, Crosstrack approach rate K2
      f32 K1_ = DEFAULT_STEERING_K1;
      f32 K2_ = DEFAULT_STEERING_K2;

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
      
      f32 currAngularVel_;
      bool startedPointTurn_;
      
      Radians prevAngle_;
      f32 angularDistExpected_;
      f32 angularDistTraversed_;
      const f32 ANGULAR_TRAVERSAL_DISTANCE_MARGIN = 0.1;
      
      // If distance to target is less than this, point turn is considered to be complete.
      const float POINT_TURN_TARGET_DIST_STOP_RAD = 0.03;

      // Maximum rotation speed of robot
      f32 maxRotationWheelSpeedDiff = 0.f;
      
      VelocityProfileGenerator vpg_;
      
      const f32 POINT_TURN_TERMINAL_VEL_RAD_PER_S = 0.4f;
      
    } // Private namespace
    
    // Private function declarations
    //Non linear version of the steering controller (For SM_PATH_FOLLOW)
    void RunLineFollowControllerNL(f32 offsetError_mm, float headingError_rad);
    void ManagePathFollow();
    void ManagePointTurn();
    void ManageDirectDrive();
    
    
    void ReInit()
    {
    }
    
    //sets the steering controller constants
    void SetGains(float k1, float k2)
    {
      PRINT("New Steering gains: k1 %f, k2 %f\n", k1, k2);
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
          PRINT("  Wheel speed adjust: (%f, %f), adjustment %f\n", *higherWheelSpeed, *lowerWheelSpeed, speedAdjust);
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
          headingError_rad = -Radians(headingError_rad + PI_F).ToFloat();
        }
      }
      
      ///////////////////////////////////////////////////////////////////////////////
      
      // Activate steering if: We are moving and the commanded speed is bigger than
      // zero (or bigger than 0+eps)
      if(SpeedController::IsVehicleStopped() == FALSE && ABS(desspeed) > SpeedController::SPEED_CONSIDER_VEHICLE_STOPPED_MM_S) {
        steering_active = TRUE;
        
      }
      
      
      // Desired behavior?  We probably only want the robot to actively steering when it's attempting to follow a path.
      // When it's not following a path, you should be able to push it around freely.
      
      //Deactivate steering if: We are not really moving and the commanded speed is zero (or smaller than 0+eps)
      if (SpeedController::IsVehicleStopped() == TRUE && ABS(desspeed) <= SpeedController::SPEED_CONSIDER_VEHICLE_STOPPED_MM_S) {
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
      PRINT(" STEERING: offsetError_mm: %f, headingError_rad: %f, curvature: %f, currSpeed: %d\n", offsetError_mm, headingError_rad, curvature, currspeed);
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
            fidx = CLIP(fidx, -5, 5);  // TODO: Loosen this up the closer we get to the block?????
            pathRadErr = CLIP(pathRadErr, -0.2, 0.2);
          }
          
          PERIODIC_PRINT(1000,"fidx: %f, distErr %f, radErr %f\n", fidx, pathDistErr, pathRadErr);
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
      //PRINT("DIRECT DRIVE %f %f\n", left_vel, right_vel);
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
    
    
    void ExecutePointTurn_Internal(f32 maxAngularVel, f32 angularAccel)
    {
      currSteerMode_ = SM_POINT_TURN;
      
      // Stop the robot if not already
      if (SpeedController::GetUserCommandedDesiredVehicleSpeed() != 0) {
        SpeedController::SetUserCommandedDesiredVehicleSpeed(0);
      }

      maxAngularVel_ = maxAngularVel;
      angularAccel_ = angularAccel;
      
      // Check that the maxAngularVel is greater than the terminal speed
      // If not, make it at least that big.
      if (ABS(maxAngularVel_) < POINT_TURN_TERMINAL_VEL_RAD_PER_S) {
        maxAngularVel_ = POINT_TURN_TERMINAL_VEL_RAD_PER_S * (maxAngularVel_ > 0 ? 1 : -1);
        PRINT("WARNING (PointTurn.TooSlow): Speeding up commanded point turn of %f rad/s to %f rad/s\n", maxAngularVel, maxAngularVel_);
      }
    
      
    }
    
    void ExecutePointTurn(f32 angularVel, f32 angularAccel)
    {
      isPointTurnWithTarget_ = false;
      ExecutePointTurn_Internal(angularVel, angularAccel);

      prevAngle_ = Localization::GetCurrentMatOrientation();
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
      SetSteerMode(SM_PATH_FOLLOW);
      currAngularVel_ = 0;
      startedPointTurn_ = false;
    }
    
    void ExecutePointTurn(f32 targetAngle, f32 maxAngularVel, f32 angularAccel, f32 angularDecel, bool useShortestDir)
    {
      isPointTurnWithTarget_ = true;
      ExecutePointTurn_Internal(maxAngularVel, angularAccel);
      
      targetRad_ = targetAngle;

      prevAngle_ = Localization::GetCurrentMatOrientation();
      f32 currAngle = prevAngle_.ToFloat();
      f32 destAngle = targetRad_.ToFloat();
      angularDistExpected_ = (targetAngle - Localization::GetCurrentMatOrientation()).ToFloat();

      angularDistTraversed_ = 0;
      
      if (ABS(angularDistExpected_) < POINT_TURN_TARGET_DIST_STOP_RAD) {
        ExitPointTurn();
#if(DEBUG_STEERING_CONTROLLER)
        PRINT("POINT TURN: Already at destination\n");
#endif
        return;
      }
      
      if (useShortestDir) {
        // Update destAngle and maxAngularVel_ so that the shortest turn is executed to reach the goal
        maxAngularVel_ = ABS(maxAngularVel) * (angularDistExpected_ > 0 ? 1 : -1);
        destAngle = currAngle + angularDistExpected_;
      } else {
        // Compute target angle that is on the appropriate side of currAngle given the maxAngularVel
        // which determines the turning direction.
        if (currAngle > destAngle && maxAngularVel_ > 0) {
          destAngle += 2*PI_F;
        } else if (currAngle < destAngle && maxAngularVel_ < 0) {
          destAngle -= 2*PI_F;
        }
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
      if (!SpeedController::IsVehicleStopped() && !startedPointTurn_) {
        f32 headingError = SpeedController::GetControllerCommandedVehicleSpeed() < 0 ? PI_F : 0;
        RunLineFollowControllerNL(0,headingError);
        return;
      }

      startedPointTurn_ = true;

      
      // Update current angular velocity
      f32 currDesiredAngle;
      vpg_.Step(currAngularVel_, currDesiredAngle);

      Radians currAngle = Cozmo::Localization::GetCurrentMatOrientation();
      
      if (isPointTurnWithTarget_) {
        
        // Compute distance to target
        float angularDistToTarget = currAngle.angularDistance(targetRad_, maxAngularVel_ < 0);
        
        
        //PRINT("currAngle: %f, targetRad: %f, AngularDist: %f, currAngularVel: %f, currDesiredAngle: %f\n",
        //          currAngle.ToFloat(), targetRad_.ToFloat(), angularDistToTarget, currAngularVel_, currDesiredAngle);
        
        
        // Check for stop condition
        f32 absAngularDistToTarget = ABS(angularDistToTarget);
        if (absAngularDistToTarget < POINT_TURN_TARGET_DIST_STOP_RAD) {
          ExitPointTurn();
  #if(DEBUG_STEERING_CONTROLLER)
          PRINT("POINT TURN: Stopping\n");
  #endif
        }
        
        
        // Check if the angular dist to target is growing
        angularDistTraversed_ += (currAngle - prevAngle_).ToFloat();
        prevAngle_ = currAngle;
        if (ABS(angularDistTraversed_) - ANGULAR_TRAVERSAL_DISTANCE_MARGIN > ABS(angularDistExpected_) ) {
          ExitPointTurn();
  #if(DEBUG_STEERING_CONTROLLER)
          PRINT("POINT TURN: Stopping because turned more than expected\n");
  #endif
        }
      }
      
      
      // Compute the velocity along the arc length equivalent of currAngularVel.
      // currAngularVel_ / PI = arcVel / (PI * R)
      s16 arcVel = (s16)(currAngularVel_ * WHEEL_DIST_HALF_MM); // mm/s


      // PID control for maintaining heading
      f32 angularDistToCurrDesiredAngle = (Radians(currDesiredAngle) - currAngle).ToFloat();
      const f32 pointTurnKp_ = 500;
      //PRINT("PT comp: arcVel %d, comp %f\n", arcVel, angularDistToCurrDesiredAngle * pointTurnKp_);
      arcVel += (s16)(angularDistToCurrDesiredAngle * pointTurnKp_);

      
      // Compute the wheel velocities
      s16 wleft = -arcVel;
      s16 wright = arcVel;

      
#if(DEBUG_STEERING_CONTROLLER)
      PRINT("POINT TURN: angularDistToTarget: %f radians, arcVel: %d mm/s\n", angularDistToTarget, arcVel);
#endif

      WheelController::SetDesiredWheelSpeeds(wleft, wright);
      
      // If target velocity is zero then we might need to stop
      if (!isPointTurnWithTarget_) {
        if (arcVel == 0 && maxAngularVel_ == 0) {
          WheelController::SetDesiredWheelSpeeds(0,0);
          ExitPointTurn();
#if(DEBUG_STEERING_CONTROLLER)
          PRINT("POINT TURN: Stopping because 0 vel reached\n");
#endif
        }
      }
    }
    
    
  } // namespace SteeringController
  } // namespace Cozmo
} // namespace Anki


