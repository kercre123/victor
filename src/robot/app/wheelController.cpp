/**
 * File: wheelController.c
 *
 * Author: Hanns Tappeiner (hanns)
 * 
 **/ 
//#include <math.h>
#include "app/debug.h"
#include "app/steeringController.h"
#include "app/trace.h"
#include "app/vehicleSpeedController.h"
#include "app/wheelController.h"
#include "hal/hal.h"
#include <stdio.h>

// Cap error_sum so that integral component of outl does not exceed say some percent of MOTOR_PWM_MAXVAL.
// e.g. 25% of 2400 = 600.  600 / wKi = 24000.  
// 24000 is also less than fix16 max value should we want to convert to fixed point.
#define MAX_WHEEL_CONTROLLER_ERROR_SUM 24000

// Gains for wheel controller
float wKp = DEFAULT_WHEEL_KP; 
float wKi = DEFAULT_WHEEL_KI; 
float wKd = DEFAULT_WHEEL_KD; 


//Returns/Sets the desired speed of the wheels (in mm/sec)
s16 desiredWheelSpeedL = 0; 
s16 desiredWheelSpeedR = 0; 


// Whether we're in coast mode (not actively running wheel controllers)
static BOOL coastMode_ = TRUE;

// One-shot coast-until-stop flag
static BOOL coastUntilStop_ = FALSE;
void DoCoastUntilStop() {coastUntilStop_ = TRUE;}

// Integral gain sums for wheel controllers
static float error_suml_ = 0; 
static float error_sumr_ = 0; 
  
// For detecting out-of-control spinning  
static u16 reverseDrivingCnt = 0;
static u16 spinCnt = 0;

// Resets the integral gain sums
void ResetWheelControllerIntegralGainSums(void);

//sets the wheel PID controller constants
void SetWheelSpeedControllerGains(float kp, float ki, float kd) { 
  wKp = kp; 
  wKi = ki; 
  wKd = kd;  
}

//Run the wheel controller: Header. See below
void RunWheelSpeedController(s16 *motorvalueoutL, s16 *motorvalueoutR);

//Manage wheel controller:
//If controller off,
  //Directly output the input but "converted" from mm/sec to motor units (!!: approx. only. See function description)
    // *motorvalueoutL =  desiredWheelSpeedL; 
    // *motorvalueoutR =  desiredWheelSpeedR;        

//This manages at a high level what the wheel speed controller needs to do
void ManageWheelSpeedController(s16 *motorvalueoutL, s16 *motorvalueoutR)
{
    //In many other case (as of now), we run the wheel controller normally
    RunWheelSpeedController(motorvalueoutL, motorvalueoutR);    
}


//Run the wheel controller
//The output motor value is in the range of -2400..2400     
void RunWheelSpeedController(s16 *motorvalueoutL, s16 *motorvalueoutR)
{ 
  if(!coastMode_ && !coastUntilStop_) {

    //Get the current wheel speed (maybe filtered?), in mm/sec
    float wspeedl = (float)GetLeftWheelSpeedFiltered(); 
    float wspeedr = (float)GetRightWheelSpeedFiltered(); 

#if(DEBUG_WHEEL_CONTROLLER)    
    fprintf(stdout, " WHEEL speeds: %f (L), %f (R)   (Curr: %d, %d)\n", wspeedl, wspeedr, GetLeftWheelSpeed(), GetRightWheelSpeed() );
    fprintf(stdout, " WHEEL desired speeds: %d (L), %d (R)\n", desiredWheelSpeedL, desiredWheelSpeedR);
#endif

    //Get the desired speed in mm/sec
    float deswspeedl = desiredWheelSpeedL; 
    float deswspeedr = desiredWheelSpeedR; 
    
    //Compute the error
    float errorl = (deswspeedl - wspeedl); 
    float errorr = (deswspeedr - wspeedr);    

    Traces16(TRACE_VAR_DESIRED_SPD_L, desiredWheelSpeedL, TRACE_MASK_MOTOR_CONTROLLER);
    Traces16(TRACE_VAR_DESIRED_SPD_R, desiredWheelSpeedR, TRACE_MASK_MOTOR_CONTROLLER);
    Tracefloat(TRACE_VAR_WSPD_FILT_L, wspeedl, TRACE_MASK_MOTOR_CONTROLLER);
    Tracefloat(TRACE_VAR_WSPD_FILT_R, wspeedr, TRACE_MASK_MOTOR_CONTROLLER);
    Tracefloat(TRACE_VAR_ERROR_L, error_suml_, TRACE_MASK_MOTOR_CONTROLLER);
    Tracefloat(TRACE_VAR_ERROR_R, error_sumr_, TRACE_MASK_MOTOR_CONTROLLER);

    // NDM: Convert to int only AFTER clamping, to avoid int overflow 
    float outl = MM_PER_SEC_TO_MOTOR_VAL( (float)(wKp * errorl) + (error_suml_ * wKi) ); 
    float outr = MM_PER_SEC_TO_MOTOR_VAL( (float)(wKp * errorr) + (error_sumr_ * wKi) ); 

#if(DEBUG_WHEEL_CONTROLLER)
    fprintf(stdout, " WHEEL error: %f (L), %f (R)   error_sum: %f (L), %f (R)\n", errorl, errorr, error_suml_, error_sumr_);
#endif

    /*
    // If commanded speed is 0 and current speed is pretty slow, suppress motor drive and don't let error accumulate.
    // This was contributing to brief slow crawls after pushing or sudden stops.
    if (GetUserCommandedCurrentVehicleSpeed() == 0 && (GetCurrentMeasuredVehicleSpeed() < WHEEL_DEAD_BAND_MM_S || (desiredWheelSpeedL < 0 && desiredWheelSpeedR < 0))) {
      error_suml_ = 0;
      error_sumr_ = 0;
      outl = 0;
      outr = 0;
      ResetVehicleSpeedControllerIntegralError();
    }
    */

    *motorvalueoutL = CLIP(outl, -MOTOR_PWM_MAXVAL, MOTOR_PWM_MAXVAL);
    *motorvalueoutR = CLIP(outr, -MOTOR_PWM_MAXVAL, MOTOR_PWM_MAXVAL);
      
    //Anti zero-crossover
    //Define a deadband above 0 where we command nothing to the wheels:
    //1) If the current wheelspeed is smaller= than the deadband
    //2) And the desired speed is smaller than the current speed
    //ATTENTION: This should work in reverse dirving as well, BUT requires
    //the encoders to return values
    if (ABS(wspeedl) <= WHEEL_DEAD_BAND_MM_S) {
      if ((wspeedl >= 0 && deswspeedl <= wspeedl) || (wspeedl < 0 && deswspeedl >= wspeedl)) {
        *motorvalueoutL = 0; 
        error_suml_ = 0;
      }
    }
    // If considered stopped, force stop
    if (ABS(deswspeedl) <= WHEEL_SPEED_COMMAND_STOPPED_MM_S) {
      *motorvalueoutL = 0; 
      error_suml_ = 0;
    }      
    
    if (ABS(wspeedr) <= WHEEL_DEAD_BAND_MM_S) {
      if ((wspeedr >= 0 && deswspeedr <= wspeedr) || (wspeedr < 0 && deswspeedr >= wspeedr)) {
        *motorvalueoutR = 0; 
        error_sumr_ = 0;
      }
    }                
    // If considered stopped, force stop
    if (ABS(deswspeedr) <= WHEEL_SPEED_COMMAND_STOPPED_MM_S) {
      *motorvalueoutR = 0; 
      error_sumr_ = 0;
    }      
    
    //Sum the error (integrate it). But ONLY, if we are not commading max output already
    //This should prevent the integral term to become to huge
    if (ABS(outl) < MOTOR_PWM_MAXVAL) {
      error_suml_ = CLIP(error_suml_ + errorl, -MAX_WHEEL_CONTROLLER_ERROR_SUM,MAX_WHEEL_CONTROLLER_ERROR_SUM);
    }
    if (ABS(outr) < MOTOR_PWM_MAXVAL) {
      error_sumr_ = CLIP(error_sumr_ + errorr, -MAX_WHEEL_CONTROLLER_ERROR_SUM,MAX_WHEEL_CONTROLLER_ERROR_SUM);
    }
  } else {
    // Coasting -- command 0 to motors
    *motorvalueoutL = 0;
    *motorvalueoutR = 0;
    error_suml_ = 0;
    error_sumr_ = 0;

    // Cancel coast until stop if we've stopped.
    if (coastUntilStop_ && GetCurrentMeasuredVehicleSpeed() == 0) {
      coastUntilStop_ = FALSE;
    }
  }

#if(DEBUG_WHEEL_CONTROLLER)
  fprintf(stdout, " WHEEL pwm: %d (L), %d (R)\n", *motorvalueoutL, *motorvalueoutR);
#endif

  //Command the computed speed (as PWM values) to the motors
  SetMotorOLSpeed(*motorvalueoutL, *motorvalueoutR);    
}


//Get the wheel speeds in mm/sec
void GetDesiredWheelSpeeds(s16 *leftws, s16 *rightws) { 
  *leftws  = desiredWheelSpeedL; 
  *rightws = desiredWheelSpeedR;
}

//Set the wheel speeds in mm/sec
void SetDesiredWheelSpeeds(s16 leftws, s16 rightws) { 
  desiredWheelSpeedL = leftws; 
  desiredWheelSpeedR = rightws;
}

//This function will command a wheel speed to the left and right wheel so that the vehicle follows a trajectory
//This will only work if the steering controller does not overwrite the values.
void utilSetVehicleOLTrajectory( u16 radius, u16 vspeed )
{

  //if the radius is zero, we can't compute the speeds and return without doing anything
  if (radius == 0) return;

  //if delta speed is positive, the left wheel is supposed to turn slower, it becomes the INNER wheel
  float leftspeed =  (float)vspeed * (1.0f - WHEEL_DIST_HALF_MM / radius);

  //if delta speed is positive, the right wheel is supposed to turn faster, it becomes the OUTER wheel
  float rightspeed = (float)vspeed * (1.0f + WHEEL_DIST_HALF_MM / radius);

  //Set the computed speeds to the wheels
  SetDesiredWheelSpeeds( (s16)leftspeed, (s16)rightspeed);
}


// Whether the wheel controller should be coasting (not actively trying to run
// wheel controllers
void SetWheelControllerCoastMode(const BOOL isOn)
{
  coastMode_ = isOn;

  if(coastMode_) {
    ResetWheelControllerIntegralGainSums();
  }
}


void ResetWheelControllerIntegralGainSums(void)
{
  error_suml_ = 0;
  error_sumr_ = 0;
}


