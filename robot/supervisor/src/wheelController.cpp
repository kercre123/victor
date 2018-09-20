/**
 * File: wheelController.c
 *
 * Author: Hanns Tappeiner (hanns)
 *
 **/

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "wheelController.h"
#include "messages.h"
#include "anki/cozmo/robot/logging.h"
#include <stdio.h>
#include <math.h>

#define DEBUG_WHEEL_CONTROLLER 0

namespace Anki {
  namespace Vector {
  namespace WheelController {

    // private members
    namespace {

      // Controller gains
      const float DEFAULT_WHEEL_KP = 0.0005f;
      const float DEFAULT_WHEEL_KI = 0.00005f;

      // Speed filtering rate
      const f32 ENCODER_FILTERING_COEFF = 0.f;  // TODO: If this seems to be ok at 0, get rid of filteredWheelSpeeds

      f32 Kp_ = DEFAULT_WHEEL_KP;
      f32 Ki_ = DEFAULT_WHEEL_KI;

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

      // Integral gain sums for wheel controllers
      f32 error_sumL_ = 0;
      f32 error_sumR_ = 0;

      // Whether or not controller should run
      bool enable_ = true;

    } // private namespace

    // Resets the integral gain sums
    void ResetIntegralGainSums(void);

    //sets the wheel PID controller constants
    void SetGains(const f32 kp, const f32 ki, const f32 maxIntegralError) {
      AnkiInfo( "WheelController.SetGains", "New gains: kp=%f, ki=%f, maxSum=%f", kp, ki, maxIntegralError);
      Kp_ = kp;
      Ki_ = ki;
      MAX_ERROR_SUM_LEFT = maxIntegralError;
    }

    void Enable()
    {
      enable_ = true;
    }

    void Disable()
    {
      if(enable_) {
        SetDesiredWheelSpeeds(0.f, 0.f);
        ResetIntegralGainSums();

        power_l_ = 0.f;
        power_r_ = 0.f;
        HAL::MotorSetPower(MotorID::MOTOR_LEFT_WHEEL, power_l_);
        HAL::MotorSetPower(MotorID::MOTOR_RIGHT_WHEEL, power_r_);

        enable_ = false;
      }
    }

    f32 ComputeWheelPower(f32 desired_speed_mmps, f32 error, f32 error_sum)
    {
      f32 x = ABS(desired_speed_mmps);

#     ifdef SIMULATOR
      f32 out_ol = x * 0.004f;
#     else
      // Piecewise linear
      f32 out_ol = 0;
      if (x > 30) {
        out_ol = (float)(0.00313 * (double)x + 0.057);  // EP2
      } else {
        // power = speed  * 0.2 power /  30 mm/s
        out_ol = 0.00666667f * x;
      }
#     endif
      
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
      const bool skipActiveControl = NEAR_ZERO(desiredWheelSpeedL_) &&
                                     NEAR_ZERO(desiredWheelSpeedR_) &&
                                     !AreWheelsPowered();

      if(!skipActiveControl) {

#if(DEBUG_WHEEL_CONTROLLER)
        AnkiDebug( "WheelController", "speeds: %f (L), %f (R)   (Curr: %d, %d)",
                filterWheelSpeedL_, filterWheelSpeedR_,
                measuredWheelSpeedL_, measuredWheelSpeedR_);
        AnkiDebug( "WheelController", "desired speeds: %f (L), %f (R)",
                desiredWheelSpeedL_, desiredWheelSpeedR_);
#endif

        //Compute the error between dessired and actual (filtered)
        f32 errorL = (desiredWheelSpeedL_ - measuredWheelSpeedL_);
        f32 errorR = (desiredWheelSpeedR_ - measuredWheelSpeedR_);

        // Compute power to command to motors
        f32 outl = ComputeWheelPower(desiredWheelSpeedL_, errorL, error_sumL_);
        f32 outr = ComputeWheelPower(desiredWheelSpeedR_, errorR, error_sumR_);


#if(DEBUG_WHEEL_CONTROLLER)
        AnkiDebug( "WheelController", "error: %f (L), %f (R)   error_sum: %f (L), %f (R)", errorL, errorR, error_sumL_, error_sumR_);
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
        if (ABS(desiredWheelSpeedL_) <= WHEEL_SPEED_COMMAND_STOPPED_MM_S && ABS(measuredWheelSpeedL_) <= WHEEL_SPEED_COMMAND_STOPPED_MM_S) {
          power_l_ = 0;
          error_sumL_ = 0;
        }


        // If considered stopped, force stop
        if (ABS(desiredWheelSpeedR_) <= WHEEL_SPEED_COMMAND_STOPPED_MM_S && ABS(measuredWheelSpeedR_) <= WHEEL_SPEED_COMMAND_STOPPED_MM_S) {
          power_r_ = 0;
          error_sumR_ = 0;
        }

        //Sum the error (integrate it). But ONLY, if we are not commading max output already
        //This should prevent the integral term to become to huge
        if (ABS(power_l_) < Vector::HAL::MOTOR_MAX_POWER) {
          error_sumL_ = error_sumL_ + errorL;
          error_sumL_ = CLIP(error_sumL_, -MAX_ERROR_SUM_LEFT,MAX_ERROR_SUM_LEFT);
        }
        if (ABS(power_r_) < Vector::HAL::MOTOR_MAX_POWER) {
          error_sumR_ = error_sumR_ + errorR;
          error_sumR_ = CLIP(error_sumR_, -MAX_ERROR_SUM_RIGHT,MAX_ERROR_SUM_RIGHT);
        }
      } else {
        // Coasting -- command 0 to motors
        power_l_ = 0;
        power_r_ = 0;
        error_sumL_ = 0;
        error_sumR_ = 0;
      }

#if(DEBUG_WHEEL_CONTROLLER)
      AnkiDebug( "WheelController", "power: %f (L), %f (R)\n", power_l_, power_r_);
#endif

      //Command the computed motor power values
      HAL::MotorSetPower(MotorID::MOTOR_LEFT_WHEEL, power_l_);
      HAL::MotorSetPower(MotorID::MOTOR_RIGHT_WHEEL, power_r_);

    } // Run()



    // Runs one step of the wheel encoder filter;
    void EncoderSpeedFilterIteration(void)
    {
      // Get encoder speed measurements
      measuredWheelSpeedL_ = Vector::HAL::MotorGetSpeed(MotorID::MOTOR_LEFT_WHEEL);
      measuredWheelSpeedR_ = Vector::HAL::MotorGetSpeed(MotorID::MOTOR_RIGHT_WHEEL);

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
    
    f32 GetCurrNoSlipBodyRotSpeed()
    {
      return (filterWheelSpeedR_ - filterWheelSpeedL_) / WHEEL_DIST_MM;
    }

    void GetFilteredWheelSpeeds(f32 &left, f32 &right)
    {
      left = filterWheelSpeedL_;
      right = filterWheelSpeedR_;
    }

    f32 GetAverageFilteredWheelSpeed()
    {
      return 0.5f * (filterWheelSpeedL_ + filterWheelSpeedR_);
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
      if (enable_) {
        desiredWheelSpeedL_ = leftws;
        desiredWheelSpeedR_ = rightws;
      } else {
        AnkiDebug("WheelController.SetDesiredWheelSpeeds.Disabled", 
                  "Ignoring speeds %f and %f while disabled",
                  leftws, rightws);
      }
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

    bool AreWheelsPowered()
    {
      return (power_l_ != 0 || power_r_ != 0);
    }

    bool AreWheelsMoving()
    {
      return !NEAR_ZERO(filterWheelSpeedL_) || !NEAR_ZERO(filterWheelSpeedR_);
    }

    void ResetIntegralGainSums(void)
    {
      error_sumL_ = 0;
      error_sumR_ = 0;
    }

  } // namespace WheelController
  } // namespace Vector
} // namespace Anki
