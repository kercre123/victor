/**
 * File: steeringController.c
 *
 * Author: Hanns Tappeiner (hanns)
 *
 **/

#include <math.h>

#include "anki/cozmo/robot/cozmoConfig.h"

#include "anki/cozmo/robot/speedController.h"
#include "anki/cozmo/robot/steeringController.h"
#include "anki/cozmo/robot/wheelController.h"
//#include "hal/portable.h"
//#include "hal/encoders.h"
#include "anki/cozmo/robot/vehicleMath.h"
#include "anki/cozmo/robot/trace.h"
#include "anki/cozmo/robot/debug.h"

#include <stdio.h>


namespace Anki {
  
  namespace SteeringController {

   
    // Private namespace
    namespace {
      //Steering gains: Heading tracking gain K1, Crosstrack approach rate K2
      f32 K1_ = DEFAULT_STEERING_K1;
      f32 K2_ = DEFAULT_STEERING_K2;
      
      bool enableAlwaysOnSteering_ = false;

      bool isInit_ = false;

    } // Private namespace
    
    void EnableAlwaysOnSteering(bool on)
    {
      enableAlwaysOnSteering_ = on;
    }
    
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
    
    //This manages at a high level what the steering controller needs to do (steer, use open loop, etc.)
    void Manage(s16 fidx, float headingError_rad)
    {
      
      // Get desired vehicle speed
      s16 desiredVehicleSpeed = SpeedController::GetControllerCommandedVehicleSpeed();
      
      //If we found a valid followline, let's run the controller
      if (fidx != INVALID_IDEAL_FOLLOW_LINE_IDX) {
        // Run controller and pass in current speed
        RunLineFollowControllerNL( fidx, headingError_rad );
        
      } else {
        // No steering intention -- pass through speed to each wheel to drive straight while in normal mode
        // we'll continue to use the previously commanded fidx
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
    void RunLineFollowControllerNL(s16 location_pix, float headingError_rad)
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
      
      
      // Do ALWAYS_ON_STEERING only when car is connected
      //if (enableAlwaysOnSteering && GetRadioSetupState() == RADIO_SETUP_CONNECTED) {
      // Desired behavior?  We probably only want the robot to actively steering when it's attempting to follow a path.
      // When it's not following a path, you should be able to push it around freely.
      if (enableAlwaysOnSteering_) {
        WheelController::SetCoastMode(false);
        steering_active = TRUE;
      } else {
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
        
      }
      
      ///////////////////////////////////////////////////////////////////////////////
      
      if (steering_active == TRUE)
      {
        //const float speed_to_pwm = 588.f; // Estimate from specs as well as value from polyFunctions.c at 1.07 m/s
        //convert speed to meters per second
        float speedmps = currspeed * 0.001f;
        float attitude = asin_fast(xtrack_speed / speedmps);
        curvature = -K1_ * (atan_fast(K2_ * xtrack_error / (speedmps + 0.2f)) + attitude);
        //We should allow this to go somewhat negative I think... but not too much
        
        Tracefloat(TRACE_VAR_ATTITUDE, attitude, TRACE_MASK_MOTOR_CONTROLLER);
        Tracefloat(TRACE_VAR_XTRACK_ERROR, xtrack_error, TRACE_MASK_MOTOR_CONTROLLER);
        Tracefloat(TRACE_VAR_XTRACK_SPEED, xtrack_speed, TRACE_MASK_MOTOR_CONTROLLER);
        Tracefloat(TRACE_VAR_SPEEDMPS, speedmps, TRACE_MASK_MOTOR_CONTROLLER);
        Tracefloat(TRACE_VAR_CURVATURE, curvature, TRACE_MASK_MOTOR_CONTROLLER);
        
        
        /*
         PROFILE_START(profRunLineFollow1);
         //float atanVal = atan(K2 * xtrack_error / speedmps);
         float atanVal = atan_fast(K2 * xtrack_error / speedmps);
         PROFILE_END(profRunLineFollow1);
         
         PROFILE_START(profRunLineFollow2);
         //float asinVal = asin(CLIP(xtrack_speed / speedmps, -1, 1));
         float asinVal = asin_fast(xtrack_speed / speedmps);
         PROFILE_END(profRunLineFollow2);
         
         curvature = -K1 * (atanVal + asinVal);
         
         Tracefloat("xtrack_error", xtrack_error, TRACE_MASK_PROFILING);
         Tracefloat("xtrack_speed", xtrack_speed, TRACE_MASK_PROFILING);
         Tracefloat("speedmps", speedmps, TRACE_MASK_PROFILING);
         */
      } else {

        curvature = 0;
 
      }
      
      
      //Convert the curvature to 1/mm
      curvature *= 1000.0f;
      
      //STOP: This will make us coast when we command 0, good for now,
      //but we might need to break later
      if (desspeed < 0) {
        curvature = 0;
      }
      
      //We are moving along a circle, so let's compute the speed for the single wheels
      //Let's interpret the delta_speed as a curvature:
      //Curvature is 1/radius
      // Commanded speeds to wheels are desired speed + offsets for curvature
      
      //if delta speed is positive, the left wheel is supposed to turn slower, it becomes the INNER wheel
      float leftspeed =  (float)desspeed - WHEEL_DIST_HALF_MM * curvature * currspeed;
      
      //if delta speed is positive, the right wheel is supposed to turn faster, it becomes the OUTER wheel
      float rightspeed = (float)desspeed + WHEEL_DIST_HALF_MM * curvature * currspeed;
      
      s16 wleft = (s16)CLIP(leftspeed,s16_MIN,s16_MAX);
      s16 wright = (s16)CLIP(rightspeed,s16_MIN,s16_MAX);
      
#if(DEBUG_STEERING_CONTROLLER)
      PRINT(" STEERING: %d (L), %d (R)\n", wleft, wright);
#endif
      
      //Command the desired wheel speeds to the wheels
      WheelController::SetDesiredWheelSpeeds(wleft, wright);
    }
    
  } // namespace SteeringController
} // namespace Anki


