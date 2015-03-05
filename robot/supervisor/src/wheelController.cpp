/**
 * File: wheelController.c
 *
 * Author: Hanns Tappeiner (hanns)
 *
 **/

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "steeringController.h"
#include "speedController.h"
#include "wheelController.h"

#include <stdio.h>

#define DEBUG_WHEEL_CONTROLLER 0

namespace Anki {
  namespace Cozmo {
  namespace WheelController {
    
    // private members
    namespace {
      
      // Cap error_sum so that integral component of outl does not exceed some percent of MOTOR_MAX_POWER.
      // e.g. 25% of 1.0 = 0.25  =>   0.25 = wKi * MAX_ERROR_SUM.
      const f32 MAX_ERROR_SUM = 5000.f;
      
      // Speed filtering rate
      const f32 ENCODER_FILTERING_COEFF = 0.9f;
      
      // Gains for wheel controller
      f32 Kp_ = DEFAULT_WHEEL_KP;
      f32 Ki_ = DEFAULT_WHEEL_KI;
      f32 Kd_ = DEFAULT_WHEEL_KD;
      
      //Returns/Sets the desired speed of the wheels (in mm/sec)
      f32 desiredWheelSpeedL_ = 0;
      f32 desiredWheelSpeedR_ = 0;
      
      // Motor power values
      f32 power_l_ = 0;
      f32 power_r_ = 0;
      
      // Encoder / Wheel Speed Filtering
      s32 measuredWheelSpeedL_ = 0;
      s32 measuredWheelSpeedR_ = 0;
      f32 filterWheelSpeedL_ = 0.f;
      f32 filterWheelSpeedR_ = 0.f;
      
      // Whether we're in coast mode (not actively running wheel controllers)
      bool coastMode_ = true;
      
      // One-shot coast-until-stop flag
      bool coastUntilStop_ = false;
      
      // Integral gain sums for wheel controllers
      f32 error_sumL_ = 0;
      f32 error_sumR_ = 0;
      
      // Whether or not controller should run
      bool enable_ = true;
      
    } // private namespace
    
    inline void DoCoastUntilStop() {coastUntilStop_ = true;}
    
    // Resets the integral gain sums
    void ResetIntegralGainSums(void);
    
    //sets the wheel PID controller constants
    void SetGains(float kp, float ki, float kd) {
      Kp_ = kp;
      Ki_ = ki;
      Kd_ = kd;
    }
    
    void Enable()
    {
      enable_ = true;
    }
    
    void Disable()
    {
      enable_ = false;
    }
    
    
    f32 ComputeLeftWheelPower(f32 desired_speed_mmps, f32 error, f32 error_sum)
    {
      // 3rd order polynomial
      // For x = speed in mm/s,
      // power = 5E-7x^3 - 0.0001x^2 + 0.0082x + 0.0149
      f32 x = ABS(desired_speed_mmps);
      f32 x2 = x*x;
      f32 x3 = x*x2;
#ifdef COZMO_TREADS
      f32 out_ol = 8.4E-7 * x3 - 0.000166336 * x2 + 0.01343098 * x;    // #2: With treads
#else
      f32 out_ol = 5.12E-7 * x3 - 0.000107221 * x2 + 0.008739278 * x;  // #1: No treads
#endif
      if (desired_speed_mmps < 0) {
        out_ol *= -1;
      }
      f32 out_corr = ( (Kp_ * error) + (error_sum * Ki_) );
      f32 out_total = out_ol + out_corr;
      return out_total;
    }

    f32 ComputeRightWheelPower(f32 desired_speed_mmps, f32 error, f32 error_sum)
    {
      // 3rd order polynomial
      // For x = speed in mm/s,
      // power = 4E-7x^3 - 0.00008x^2 + 0.0072x + 0.0203
      f32 x = ABS(desired_speed_mmps);
      f32 x2 = x*x;
      f32 x3 = x*x2;
#ifdef COZMO_TREADS
      f32 out_ol = 4.824E-7 * x3 - 8.98123E-5 * x2 + 0.007008705 * x;        // #2: With treads
#else
      f32 out_ol = 3.97E-7 * x3 - 0.000084032 * x2 + 0.008001138 * x;   // #1: No treads
#endif
      if (desired_speed_mmps < 0) {
        out_ol *= -1;
      }
      f32 out_corr = ( (Kp_ * error) + (error_sum * Ki_) );
      f32 out_total = out_ol + out_corr;
      return out_total;
      
    }
    
    
    //Run the wheel controller
    void Run()
    {
      if(1) {
//      if(!coastMode_ && !coastUntilStop_) {
        
#if(DEBUG_WHEEL_CONTROLLER)
        PRINT(" WHEEL speeds: %f (L), %f (R)   (Curr: %d, %d)\n",
                filterWheelSpeedL_, filterWheelSpeedR_,
                measuredWheelSpeedL_, measuredWheelSpeedR_);
        PRINT(" WHEEL desired speeds: %f (L), %f (R)\n",
                desiredWheelSpeedL_, desiredWheelSpeedR_);
#endif
        
        //Compute the error between dessired and actual (filtered)
        f32 errorL = (desiredWheelSpeedL_ - filterWheelSpeedL_);
        f32 errorR = (desiredWheelSpeedR_ - filterWheelSpeedR_);
        
        // Compute power to command to motors
        f32 outl = ComputeLeftWheelPower(desiredWheelSpeedL_, errorL, error_sumL_);
        f32 outr = ComputeRightWheelPower(desiredWheelSpeedR_, errorR, error_sumR_);
        
        
#if(DEBUG_WHEEL_CONTROLLER)
        PRINT(" WHEEL error: %f (L), %f (R)   error_sum: %f (L), %f (R)\n", errorL, errorR, error_sumL_, error_sumR_);
#endif
        
        /*
         // If commanded speed is 0 and current speed is pretty slow, suppress motor drive and don't let error accumulate.
         // This was contributing to brief slow crawls after pushing or sudden stops.
         if (GetUserCommandedCurrentVehicleSpeed() == 0 && (GetCurrentMeasuredVehicleSpeed() < WHEEL_DEAD_BAND_MM_S || (desiredWheelSpeedL < 0 && desiredWheelSpeedR < 0))) {
         error_suml_ = 0;
         error_sumr_ = 0;
         outl = 0;
         outr = 0;
         ResetSpeedControllerIntegralError();
         }
         */
        
        power_l_ = CLIP(outl, -HAL::MOTOR_MAX_POWER, HAL::MOTOR_MAX_POWER);
        power_r_ = CLIP(outr, -HAL::MOTOR_MAX_POWER, HAL::MOTOR_MAX_POWER);
        
        
        // If considered stopped, force stop
        if (ABS(desiredWheelSpeedL_) <= WHEEL_SPEED_COMMAND_STOPPED_MM_S) {
          power_l_ = 0;
          error_sumL_ = 0;
        }
        
        
        // If considered stopped, force stop
        if (ABS(desiredWheelSpeedR_) <= WHEEL_SPEED_COMMAND_STOPPED_MM_S) {
          power_r_ = 0;
          error_sumR_ = 0;
        }
        
        //Sum the error (integrate it). But ONLY, if we are not commading max output already
        //This should prevent the integral term to become to huge
        if (ABS(power_l_) < Cozmo::HAL::MOTOR_MAX_POWER) {
          error_sumL_ = CLIP(error_sumL_ + errorL, -MAX_ERROR_SUM,MAX_ERROR_SUM);
        }
        if (ABS(power_r_) < Cozmo::HAL::MOTOR_MAX_POWER) {
          error_sumR_ = CLIP(error_sumR_ + errorR, -MAX_ERROR_SUM,MAX_ERROR_SUM);
        }
      } else {
        // Coasting -- command 0 to motors
        power_l_ = 0;
        power_r_ = 0;
        error_sumL_ = 0;
        error_sumR_ = 0;
        
        // Cancel coast until stop if we've stopped.
        if (coastUntilStop_ &&
            SpeedController::GetCurrentMeasuredVehicleSpeed() == 0) {
          coastUntilStop_ = FALSE;
        }
      }
      
#if(DEBUG_WHEEL_CONTROLLER)
      PRINT(" WHEEL power: %f (L), %f (R)\n", power_l_, power_r_);
#endif
      
      //Command the computed motor power values
      HAL::MotorSetPower(HAL::MOTOR_LEFT_WHEEL, power_l_);
      HAL::MotorSetPower(HAL::MOTOR_RIGHT_WHEEL, power_r_);

      
    } // Run()
    
    
    
    // Runs one step of the wheel encoder filter;
    void EncoderSpeedFilterIteration(void)
    {
      // Get encoder speed measurements
      measuredWheelSpeedL_ = Cozmo::HAL::MotorGetSpeed(HAL::MOTOR_LEFT_WHEEL);
      measuredWheelSpeedR_ = Cozmo::HAL::MotorGetSpeed(HAL::MOTOR_RIGHT_WHEEL);
      
      filterWheelSpeedL_ = (measuredWheelSpeedL_ *
                       (1.0f - ENCODER_FILTERING_COEFF) +
                       (filterWheelSpeedL_ * ENCODER_FILTERING_COEFF));
      filterWheelSpeedR_ = (measuredWheelSpeedR_ *
                       (1.0f - ENCODER_FILTERING_COEFF) +
                       (filterWheelSpeedR_ * ENCODER_FILTERING_COEFF));
      
    } // EncoderSpeedFilterIteration()
    
    
    //This manages at a high level what the wheel speed controller needs to do
    void Manage()
    {
      //In many other case (as of now), we run the wheel controller normally
      if (enable_) {
        Run();
      }
      
      EncoderSpeedFilterIteration();
    }
    
    void GetFilteredWheelSpeeds(f32 &left, f32 &right)
    {
      left = filterWheelSpeedL_;
      right = filterWheelSpeedR_;
    }
    
    //Get the wheel speeds in mm/sec
    void GetDesiredWheelSpeeds(f32 &leftws, f32 &rightws)
    {
      leftws  = desiredWheelSpeedL_;
      rightws = desiredWheelSpeedR_;
    }
    
    //Set the wheel speeds in mm/sec
    void SetDesiredWheelSpeeds(f32 leftws, f32 rightws)
    {
      desiredWheelSpeedL_ = leftws;
      desiredWheelSpeedR_ = rightws;
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
    void SetCoastMode(const bool isOn)
    {
      coastMode_ = isOn;
      
      if(coastMode_) {
        ResetIntegralGainSums();
      }
    }

    bool AreWheelsPowered()
    {
      return (power_l_ != 0 || power_r_ != 0);
    }
    
    
    void ResetIntegralGainSums(void)
    {
      error_sumL_ = 0;
      error_sumR_ = 0;
    }
    
  } // namespace WheelController
  } // namespace Cozmo
} // namespace Anki
