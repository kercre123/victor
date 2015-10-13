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
#include "messages.h"
#include <stdio.h>

#define DEBUG_WHEEL_CONTROLLER 0

namespace Anki {
  namespace Cozmo {
  namespace WheelController {
    
    // private members
    namespace {
      
      // Controller gains
      const float DEFAULT_WHEEL_KP = 0.0005f;
      const float DEFAULT_WHEEL_KI = 0.00005f;
      
      // Speed filtering rate
      const f32 ENCODER_FILTERING_COEFF = 0.9f;
      
      
      f32 Kp_l_ = DEFAULT_WHEEL_KP;
      f32 Ki_l_ = DEFAULT_WHEEL_KI;
      
      f32 Kp_r_ = DEFAULT_WHEEL_KP;
      f32 Ki_r_ = DEFAULT_WHEEL_KI;
      
      f32 MAX_ERROR_SUM_LEFT = 5000.f;
      f32 MAX_ERROR_SUM_RIGHT = 5000.f;
      
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
    void SetGains(const f32 kpLeft, const f32 kiLeft, const f32 maxIntegralErrorLeft,
                  const f32 kpRight, const f32 kiRight, const f32 maxIntegralErrorRight) {
      PRINT("New wheelController gains: kLeft (p=%f, i=%f, maxSum=%f) kRight (p=%f, i=%f maxSum=%f)\n", kpLeft, kiLeft, maxIntegralErrorLeft, kpRight, kiRight, maxIntegralErrorRight);
      Kp_l_ = kpLeft;
      Ki_l_ = kiLeft;
      MAX_ERROR_SUM_LEFT = maxIntegralErrorLeft;
      
      Kp_r_ = kpRight;
      Ki_r_ = kiRight;
      MAX_ERROR_SUM_RIGHT = maxIntegralErrorRight;
    }
    
    void Enable()
    {
      enable_ = true;
    }
    
    void Disable()
    {
      if(enable_) {
        enable_ = false;
        
        ResetIntegralGainSums();
        power_l_ = 0.f;
        power_r_ = 0.f;
        
        HAL::MotorSetPower(HAL::MOTOR_LEFT_WHEEL, power_l_);
        HAL::MotorSetPower(HAL::MOTOR_RIGHT_WHEEL, power_r_);
      }
    }
    
    
    f32 ComputeLeftWheelPower(f32 desired_speed_mmps, f32 error, f32 error_sum)
    {
      f32 x = ABS(desired_speed_mmps);
      
#     ifdef SIMULATOR
      f32 out_ol = x * 0.004;
#     else
      // Piecewise linear
      f32 out_ol = 0;
      if (x > 10) {
        out_ol = (float)(0.003319096 * (double)x + 0.10109226);
      } else {
        // power = speed  * 0.2 power /  10 mm/s
        out_ol = 0.02f * x;
      }
#     endif
      
      if (desired_speed_mmps < 0) {
        out_ol *= -1;
      }
      f32 out_corr = ( (Kp_l_ * error) + (error_sum * Ki_l_) );
      f32 out_total = out_ol + out_corr;
      return out_total;
    }

    f32 ComputeRightWheelPower(f32 desired_speed_mmps, f32 error, f32 error_sum)
    {
      f32 x = ABS(desired_speed_mmps);
      
#     ifdef SIMULATOR
      f32 out_ol = x * 0.004f;
#     else
      // Piecewise linear
      f32 out_ol = 0;
      if (x > 10) {
        out_ol = (float)(0.00336296136 * (double)x + 0.10104465883);
      } else {
        // power = speed  * 0.2 power /  10 mm/s
        out_ol = 0.02f * x;
      }
#     endif
      
      if (desired_speed_mmps < 0) {
        out_ol *= -1;
      }
      f32 out_corr = ( (Kp_r_ * error) + (error_sum * Ki_r_) );
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
          error_sumL_ = CLIP(error_sumL_ + errorL, -MAX_ERROR_SUM_LEFT,MAX_ERROR_SUM_LEFT);
        }
        if (ABS(power_r_) < Cozmo::HAL::MOTOR_MAX_POWER) {
          error_sumR_ = CLIP(error_sumR_ + errorR, -MAX_ERROR_SUM_RIGHT,MAX_ERROR_SUM_RIGHT);
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
