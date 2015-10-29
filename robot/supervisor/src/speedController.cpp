/**
 * File: speedController.c
 * Author: Hanns Tappeiner (hanns@anki.com)
 * Date: 07/11/2012
 *
 * The vehicle needs to keep a forward speed as close as
 * possible to the speed commanded by the user (user means BTLE
 * Master, e.g. iPhone). There are multiple ways to
 * measure/control that speed:
 *
 * 1) The vehicle speed is measured/controlled by
 * measuring/controlling the two wheel speeds (encoders).
 * Assuming no slip, this should be very accurate and we
 * can do it many times per second. We get new measurments
 * every ~4.3mm (in version v4.1 of the car) of forward travel.
 * Also, this should work well during lane changes. With slip,
 * we loose precision, but it is probably the best way of
 * estimating forward speed at the micro level.
 *
 * 2) The vehicle speed is measured/controlled by
 * measuring/controlling the forward progress looking at track
 * codes. This is not precise at high speeds and also does not
 * really work during lane changes.
 *
 * For now, we will exclusively rely on the encoders to control
 * forward speed. At some point we MIGHT incorporate the
 * global forward speed measured via the road codes as well, but
 * only if it turns out that we need it.
 **/

//#include <math.h>
//#include "hal/portable.h"
#include <assert.h>
#include "anki/types.h"
#include "speedController.h"
#include "wheelController.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "anki/common/robot/utilities_c.h"


#define DEBUG_SPEED_CONTROLLER 0

namespace Anki {
  namespace Cozmo {
  namespace SpeedController {
    
    
// #pragma mark --- "Member Variables" ---
    namespace {
      // The target desired speed the user commanded to the car [mm/sec].
      // This is our eventual goal for the vehicle speed, given enough time for acceleration
      s16 userCommandedDesiredVehicleSpeed_ = 0;
      
      // The current speed according to the target speed and acceleration set by the user. [mm/sec]
      // This is our speed goal for the current iteration. This is what the PID controller will use
      f32 userCommandedCurrentVehicleSpeed_ = 0;
      
      // The acceleration the user commanded to the car [mm/sec^2]
      const u16 DEFAULT_ACCEL_MMPS2 = 200;
      u16 userCommandedAcceleration_ = DEFAULT_ACCEL_MMPS2;

      // The deceleration the user commanded to the car [mm/sec^2]
      const u16 DEFAULT_DECEL_MMPS2 = 1000;
      u16 userCommandedDeceleration_ = DEFAULT_DECEL_MMPS2;

      
      // The controller needs to regulate the speed of the car "around" the user commanded speed [mm/sec]
      s16 controllerCommandedVehicleSpeed_ = 0;
      
      s32 errorsum_ = 0;
      
    } // private namespace
    
// #pragma mark --- Method Implementations ---
    
    // Forward declaration
    void Run(s16 desVehicleSpeed);
    
    // Getters and Setters
    void SetUserCommandedCurrentVehicleSpeed(s16 ucspeed)
    {
      userCommandedCurrentVehicleSpeed_ = ucspeed;
    }
    
    s16 GetUserCommandedCurrentVehicleSpeed(void)
    {
      return (s16)userCommandedCurrentVehicleSpeed_;
    }
    
    void SetUserCommandedDesiredVehicleSpeed(s16 ucspeed)
    {
      userCommandedDesiredVehicleSpeed_ = ucspeed;
      
      // If the current measured speed and the current user commanded speed
      // are on the same side of the user desired speed and the measured speed
      // is closer to the user desired speed than the current user commanded speed, then set the
      // current user commanded speed to be the current measured speed.
      // It makes no sense to try to slow down before speeding up!
      s16 desiredAndCurrentSpeedDiff = userCommandedDesiredVehicleSpeed_ - userCommandedCurrentVehicleSpeed_;
      s16 desiredAndMeasuredSpeedDiff = userCommandedDesiredVehicleSpeed_ - GetCurrentMeasuredVehicleSpeed();
      if ( signbit(desiredAndCurrentSpeedDiff) == signbit(desiredAndMeasuredSpeedDiff)
          && ABS(desiredAndCurrentSpeedDiff) > ABS(desiredAndMeasuredSpeedDiff) ) {
        userCommandedCurrentVehicleSpeed_ = GetCurrentMeasuredVehicleSpeed();
      }
      
      // Similarly, if the current measured speed and the current user commanded speed
      // are on opposite sides of the user desired speed, set the user commanded speed
      // to be the desired speed.
      if ( signbit(desiredAndCurrentSpeedDiff) != signbit(desiredAndMeasuredSpeedDiff) ) {
        userCommandedCurrentVehicleSpeed_ = userCommandedDesiredVehicleSpeed_;
      }
    }
    
    s16 GetUserCommandedDesiredVehicleSpeed(void)
    {
      return userCommandedDesiredVehicleSpeed_;
    }
    
    void SetBothDesiredAndCurrentUserSpeed(s16 ucspeed)
    {
      userCommandedDesiredVehicleSpeed_ = ucspeed;
      userCommandedCurrentVehicleSpeed_ = ucspeed;
    }
    
    void SetUserCommandedAcceleration(u16 ucAccel)
    {
      userCommandedAcceleration_ = ucAccel;
    }
    
    u16 GetUserCommandedAcceleration(void)
    {
      return userCommandedAcceleration_;
    }
    
    void SetUserCommandedDeceleration(u16 ucDecel)
    {
      userCommandedDeceleration_ = ucDecel;
    }
    
    u16 GetUserCommandedDeceleration(void)
    {
      return userCommandedDeceleration_;
    }
    
    void SetControllerCommandedVehicleSpeed(s16 ccspeed)
    {
      controllerCommandedVehicleSpeed_ = ccspeed;
    }
    
    s16 GetControllerCommandedVehicleSpeed(void)
    {
      return controllerCommandedVehicleSpeed_;
    }
    
    //This tells us how fast the vehicle is driving right now in mm/sec
    s16 GetCurrentMeasuredVehicleSpeed(void)
    {
      f32 filteredSpeedL, filteredSpeedR;
      WheelController::GetFilteredWheelSpeeds(filteredSpeedL, filteredSpeedR);
      
      // TODO: are we sure this should be returned as s16?
      return static_cast<s16>(0.5f*(filteredSpeedL + filteredSpeedR));
    }
    
    void RunAccelerationUpdate(void)
    {
      if (userCommandedDesiredVehicleSpeed_ > userCommandedCurrentVehicleSpeed_) {
        // Go faster
        userCommandedCurrentVehicleSpeed_ += (f32)userCommandedAcceleration_ * Cozmo::CONTROL_DT;
        userCommandedCurrentVehicleSpeed_ = MIN(userCommandedDesiredVehicleSpeed_, userCommandedCurrentVehicleSpeed_);
      } else if (userCommandedDesiredVehicleSpeed_ < userCommandedCurrentVehicleSpeed_) {
        // Go slower
        userCommandedCurrentVehicleSpeed_ -= (f32)userCommandedDeceleration_ * Cozmo::CONTROL_DT;
        userCommandedCurrentVehicleSpeed_ = MAX(userCommandedDesiredVehicleSpeed_, userCommandedCurrentVehicleSpeed_);
      }
      
      //PRINT("RunAccelUpdate: accel/decel = (%d,%d), commandedSpeed = %f, desiredSpeed = %d\n",
      //      userCommandedAcceleration_, userCommandedDeceleration_,
      //      userCommandedCurrentVehicleSpeed_, userCommandedDesiredVehicleSpeed_);
    }
    
    void Manage(void)
    {
      //For now, the only thing we do is to set the controller commanded vehicle speed to whatever the user commanded
      //Later we will (potentially) change this to a PI contontroller trying to achieve the speed we want
      
      RunAccelerationUpdate();
      Run(GetUserCommandedCurrentVehicleSpeed());
      
    }
    
    
    bool IsVehicleStopped(void)
    {
      //If the left and the right encoder are not moving (or moving REALLY slow), we are stopped
      f32 wheelSpeedL, wheelSpeedR;
      WheelController::GetFilteredWheelSpeeds(wheelSpeedL, wheelSpeedR);
      
      if(ABS(wheelSpeedL) < WheelController::WHEEL_SPEED_CONSIDER_STOPPED_MM_S &&
         ABS(wheelSpeedR) < WheelController::WHEEL_SPEED_CONSIDER_STOPPED_MM_S ) {
        return TRUE;
      } else{
        return FALSE;
      }
      
    }
    
    // Integral speed controller.
    // Adjusts contollerCommandedVehicleSpeed according to given desiredVehicleSpeed.
    void Run(s16 desVehicleSpeed)
    {
#if (0)
      s32 currspeed = GetCurrentMeasuredVehicleSpeed();
      
      // Get the current error
      s32 currerror = desVehicleSpeed - currspeed;
      
      // We run 500 times per second
      // We get a new reading every 4.3mm
      // Lets say we try to drive 1.5m/s, but only drive 1.4 (on average)
      // Over 50 cycles (1/10 second), our sum will be (100mm * 50) = 5000 ||| 50
      // Over 500 cycles (in one second), our sum will be (100mm * 500) = 50000 ||| 500  (some of those number are too big for signed short!!!!)
      controllerCommandedVehicleSpeed_ = (desVehicleSpeed + (currerror * KP) +
                                          (errorsum_ * KI));
      
      
      // Update and cap errorsum so that it doesn't get too huge.
      errorsum_ += currerror;
      errorsum_ = CLIP(errorsum_, -MAX_ERRORSUM, MAX_ERRORSUM);
      
      //Anti zero-crossover
      //Define a deadband above 0 where we command nothing to the wheels:
      //1) If the current wheelspeed is smaller= than the deadband
      //2) And the desired speed is smaller than the current speed
      //ATTENTION: This should work in reverse dirving as well, BUT requires
      //the encoders to return values
      if (ABS(currspeed) <= WheelController::WHEEL_DEAD_BAND_MM_S) {
        if ((currspeed >= 0 && desVehicleSpeed <= currspeed) || (currspeed < 0 && desVehicleSpeed >= currspeed)) {
          controllerCommandedVehicleSpeed_ = desVehicleSpeed;
          errorsum_ = 0;
          //ResetWheelControllerIntegralGainSums();
        }
      }
      
      // If considered stopped, force stop
      if (ABS(desVehicleSpeed) <= WheelController::WHEEL_SPEED_COMMAND_STOPPED_MM_S) {
        controllerCommandedVehicleSpeed_ = desVehicleSpeed;
        errorsum_ = 0;
        //ResetWheelControllerIntegralGainSums();
      }
      
#if(DEBUG_SPEED_CONTROLLER)
      PRINT(" SPEED_CTRL: userDesSpeed: %d, userCurrSpeed: %f, measSpeed: %d, userAccel: %d, controllerSpeed: %d, currError: %d, errorSum: %d\n", userCommandedDesiredVehicleSpeed_, userCommandedCurrentVehicleSpeed_, GetCurrentMeasuredVehicleSpeed(), userCommandedAcceleration_, controllerCommandedVehicleSpeed_, currerror, errorsum_);
#endif
      
#else
      // KEVIN 2012_12_18: Disabled integral vehicle controller per Gabe's recommendations.
      // He says increasing Ki of wheel speed controller is sufficient.
      controllerCommandedVehicleSpeed_ = desVehicleSpeed;
      //s32 currerror = 0;
#endif
    }
    
    
    void ResetIntegralError() {
      errorsum_ = 0;
    }
    
  } // namespace SpeedController
  } // namespace Cozmo
} // namespace Anki


