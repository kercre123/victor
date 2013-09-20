/**
 * File: steeringController.c
 *
 * Author: Hanns Tappeiner (hanns)
 * 
 **/

#include <math.h>
#include "app/vehicleSpeedController.h"
#include "app/steeringController.h"
#include "app/wheelController.h"
//#include "hal/portable.h"
//#include "hal/encoders.h"
#include "app/vehicleMath.h"
#include "app/trace.h"
#include "cozmoConfig.h"

#include <stdio.h>

//Steering gains: Heading tracking gain K1, Crosstrack approach rate K2
float K1 = DEFAULT_STEERING_K1;
float K2 = DEFAULT_STEERING_K2;


static BOOL enableAlwaysOnSteering = FALSE;
void EnableAlwaysOnSteering(BOOL on) {enableAlwaysOnSteering = on;}


static u8 isInit = 0;
void ReInitSteeringController() {
  isInit = 0;
}


//sets the steering controller constants
void SetSteeringControllerGains(float k1, float k2) {
  K1 = k1;
  K2 = k2;
}


//This manages at a high level what the steering controller needs to do (steer, use open loop, etc.)
void ManageSteeringController(s16 fidx) {
   
  // Get desired vehicle speed
  s16 desiredVehicleSpeed = GetControllerCommandedVehicleSpeed();

  
  //If we found a valid followline, let's run the controller
  if (fidx != INVALID_IDEAL_FOLLOW_LINE_IDX) {
        // Run controller and pass in current speed
        RunLineFollowControllerNL( fidx );

  } else {
        // No steering intention -- pass through speed to each wheel to drive straight while in normal mode
        // we'll continue to use the previously commanded fidx
  }
  
      
 
}

#define M_PER_MM  (0.001f)  /**< Meters per millimeter */


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
void RunLineFollowControllerNL(s16 location_pix) {

  //We only steer in certain cases, for example if the car is supposed to move
  static BOOL steering_active = FALSE;

  // Measured error in meters
  //float xtrack_error = (location_pix - LIN_ARRAY_CENTER_PIXEL) * LINE_ARRAY_MM_PER_PIX * M_PER_MM; // Crosstrack error (m)
  float xtrack_error = location_pix * M_PER_MM; // Crosstrack error (m)

  float xtrack_speed;
  static float xtrack_error_last; // Holds value of previous crosstrack error for next loop iteration
  //static u8 isInit = 0; // (initialization flag, false on first function call only)

  // Initization
  if (isInit == 0) {
    xtrack_speed = 0.f; // because xtrack_error_last never had a chance to be set
    isInit = 1;
  } else {
    xtrack_speed = (xtrack_error - xtrack_error_last) / CONTROL_DT;
  }
  xtrack_error_last = xtrack_error;

  // Control law
  float curvature = 0;

  //Get the current vehicle speed (based on encoder values etc.) in mm/sec
  s16 currspeed = GetCurrentMeasuredVehicleSpeed();
  //Get the desired vehicle speed (the one the user commanded to the car)
  s16 desspeed = GetControllerCommandedVehicleSpeed();


  ///////////////////////////////////////////////////////////////////////////////

  // Activate steering if: We are moving and the commanded speed is bigger than
  // zero (or bigger than 0+eps)
  if(IsVehicleStopped() == FALSE && desspeed > SPEED_CONSIDER_VEHICLE_STOPPED_MM_S) {
    steering_active = TRUE;

  }


  // Do ALWAYS_ON_STEERING only when car is connected
  //if (enableAlwaysOnSteering && GetRadioSetupState() == RADIO_SETUP_CONNECTED) {
  // Desired behavior?  We probably only want the robot to actively steering when it's attempting to follow a path.
  // When it's not following a path, you should be able to push it around freely.
  if (enableAlwaysOnSteering) {  
    SetWheelControllerCoastMode(FALSE);
    steering_active = TRUE;
  } else {
    //Deactivate steering if: We are not really moving and the commanded speed is zero (or smaller than 0+eps)
    if (IsVehicleStopped() == TRUE && desspeed <= SPEED_CONSIDER_VEHICLE_STOPPED_MM_S) {
      steering_active = FALSE;

      // Set wheel controller coast mode as we finish decelerating to 0
      SetWheelControllerCoastMode(TRUE);
    }

    // If we're commanding any non-zero speed, don't coast
    if(desspeed > SPEED_CONSIDER_VEHICLE_STOPPED_MM_S) {
      SetWheelControllerCoastMode(FALSE);
    }

  }

  ///////////////////////////////////////////////////////////////////////////////
  
  if (steering_active == TRUE)
  {
    //const float speed_to_pwm = 588.f; // Estimate from specs as well as value from polyFunctions.c at 1.07 m/s
    //convert speed to meters per second
    float speedmps = currspeed * 0.001f;
    float attitude = asin_fast(xtrack_speed / speedmps);
    curvature = -K1 * (atan_fast(K2 * xtrack_error / (speedmps + 0.2f)) + attitude);
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
  } else curvature = 0;  

  

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
  printf(" STEERING: %d (L), %d (R)\n", wleft, wright);
#endif
	
  //Command the desired wheel speeds to the wheels
  SetDesiredWheelSpeeds(wleft, wright);
}


