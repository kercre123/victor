/**
 * File: wheelController.h
 *
 * Author: Hanns Tappeiner (hanns)
 *
 *
 **/

#ifndef WHEEL_CONTROLLER_H_
#define WHEEL_CONTROLLER_H_

#include "anki/cozmo/shared/cozmoConfig.h"

namespace Anki {
  namespace Cozmo {
  namespace WheelController {
    
    // Controller gains for generating wheel power commands.
    // Approximating power-speed model with piecewise function with 2 linear segments.
    // One slope from a power of 0 to a transition power, the other from the transition power to 1.0.
    //
    // Piecewise trendline:
    //    Two pieces: 1) from 0 power at 0 speed to TRANSITION_POWER at TRANSITION_SPEED (slope = LOW_OPEN_LOOP_GAIN)
    //                2) from TRANSITION_POWER at TRANSITION_SPEED to 1.0 power  with slope of HIGH_OPEN_LOOP_GAIN
    
    // Point at which open loop speed-power trendlines meet
    const f32 TRANSITION_SPEED = 25.0f;  // mm/s
    const f32 TRANSITION_POWER = 0.30f;  // wheel motor power
    
    // Slope of power:speed in upper line.
    // Obtained from plotting average open loop response of both wheels.
    const f32 HIGH_OPEN_LOOP_GAIN = (1.0f - TRANSITION_POWER) / (MAX_WHEEL_SPEED_MMPS - TRANSITION_SPEED);
    
    // open loop power = speed * HIGH_OPEN_LOOP_GAIN + HIGH_OPEN_LOOP_OFFSET
    const f32 HIGH_OPEN_LOOP_OFFSET = TRANSITION_POWER - (TRANSITION_SPEED * HIGH_OPEN_LOOP_GAIN);
    
    // Slope of power:speed in lower line
    const f32 LOW_OPEN_LOOP_GAIN = TRANSITION_POWER / TRANSITION_SPEED;
    
    // Controller gains
    const float DEFAULT_WHEEL_KP = 0.0005f;
    const float DEFAULT_WHEEL_KI = 0.00005f;
    const float DEFAULT_WHEEL_KD = 0.0f;
    const float DEFAULT_WHEEL_LOW_KP = 0.005f;
    const float DEFAULT_WHEEL_LOW_KI = 0.0001f;
    
    const float WHEEL_SPEED_COMMAND_STOPPED_MM_S = 2.0;
    
    //If we drive slower than this, the vehicle is stopped
    const float WHEEL_SPEED_CONSIDER_STOPPED_MM_S = 2; // TODO: float or int?
    
    //If we are in this deadband, we won't command anythign to the wheels if we are trying to slow down
    const float WHEEL_DEAD_BAND_MM_S = 2; // TODO: float or int?
    
    
    //Set the gains for the PID controllers (same gains for left and right wheel)
    void SetGains(float kp, float ki, float kd);

    // Enable/Disable the wheel controller.
    // Mostly only useful for test mode or special simluation modes.
    void Enable();
    void Disable();
    
    //This manages at a high level what the wheel speed controller needs to do
    void Manage();
    
    //Sets/Gets the desired speeds for the wheels (in mm/sec forward speed)
    void GetDesiredWheelSpeeds(f32 &leftws, f32 &rightws);
    void SetDesiredWheelSpeeds(f32 leftws, f32 rightws);
    
    void GetFilteredWheelSpeeds(f32 &left, f32 &right);
    
    //This function will command a wheel speed to the left and right wheel so that the vehicle follows a trajectory
    //This will only work if the steering controller does not overwrite the values.
    void utilSetVehicleOLTrajectory( u16 radius, u16 vspeed );
    
    // Whether the wheel controller should be coasting (not actively trying to run
    // wheel controllers
    void SetCoastMode(bool isOn);
    
    bool AreWheelsPowered();
    
    void ResetIntegralGainSums(void);
    
    
    // Call this to suppress drive to motors until car is stopped.
    // Used to make sure car doesn't keep driving when it delocalizes.
    void DoCoastUntilStop(void);
    
  } // namespace WheelController
  } // namespace Cozmo
} // namespace Anki


#endif
