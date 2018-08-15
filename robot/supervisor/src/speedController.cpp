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

#include <math.h>
#include "coretech/common/shared/types.h"
#include "speedController.h"
#include "wheelController.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/robot/logging.h"

#define DEBUG_SPEED_CONTROLLER 0

namespace Anki {
  namespace Vector {
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
    
    void SetDefaultAccelerationAndDeceleration()
    {
      userCommandedAcceleration_ = DEFAULT_ACCEL_MMPS2;
      userCommandedDeceleration_ = DEFAULT_DECEL_MMPS2;
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
      return (s16)(0.5f*(filteredSpeedL + filteredSpeedR));
    }

    void RunAccelerationUpdate(void)
    {
      if (userCommandedDesiredVehicleSpeed_ > userCommandedCurrentVehicleSpeed_) {
        // Go faster
        userCommandedCurrentVehicleSpeed_ += (f32)userCommandedAcceleration_ * Vector::CONTROL_DT;
        userCommandedCurrentVehicleSpeed_ = MIN(userCommandedDesiredVehicleSpeed_, userCommandedCurrentVehicleSpeed_);
      } else if (userCommandedDesiredVehicleSpeed_ < userCommandedCurrentVehicleSpeed_) {
        // Go slower
        userCommandedCurrentVehicleSpeed_ -= (f32)userCommandedDeceleration_ * Vector::CONTROL_DT;
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
      controllerCommandedVehicleSpeed_ = GetUserCommandedCurrentVehicleSpeed();

    }

  } // namespace SpeedController
  } // namespace Vector
} // namespace Anki
