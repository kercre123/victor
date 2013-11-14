/**
 * File: wheelController.h
 *
 * Author: Hanns Tappeiner (hanns)
 *
 *
 **/

#ifndef WHEEL_CONTROLLER_H_
#define WHEEL_CONTROLLER_H_

namespace Anki {
  namespace Cozmo {
  namespace WheelController {
    
    // Controller gains for generating wheel power commands.
    // Approximating power-speed model with piecewise function with 2 linear segments.
    // One slope from a power of 0 to a transition power, the other from the transition power to 1.0.
    //
    // Piecwise trendline:
    //    HIGH PIECE: for speed >= 80
    //    speed = 315.4 * power - 45  => power = 0.00317 * (speed + 45)
    //
    //    LOW PIECE: for speed < 80
    //    transition power = ( 80 + 45 ) / 315.4 = 0.396322
    //    speed = (80 / 0.396322) * power =>  power = 0.004954 * speed
    const f32 TRANSITION_POWER = 80;
    const f32 HIGH_OPEN_LOOP_OFFSET = 45;
    const f32 HIGH_OPEN_LOOP_GAIN = 0.00317;
    const f32 LOW_OPEN_LOOP_GAIN = 0.004954;
    const float DEFAULT_WHEEL_KP = 0.0005f;
    const float DEFAULT_WHEEL_KI = 0.00005f;
    const float DEFAULT_WHEEL_KD = 0.0f;
    const float DEFAULT_WHEEL_LOW_KP = 0.005f;
    const float DEFAULT_WHEEL_LOW_KI = 0.0001f;
    
    
    const float WHEEL_SPEED_COMMAND_STOPPED_MM_S = 2.0;
    
    //How fast (in mm/sec) can a wheel spin at max
    const float MAX_WHEEL_SPEED_MM_S = 1000; // TODO: float or int?
    const float MIN_WHEEL_SPEED_MM_S = 0;    // TODO: float or int?
    
    //If we drive slower than this, the vehicle is stopped
    const float WHEEL_SPEED_CONSIDER_STOPPED_MM_S = 2; // TODO: float or int?
    
    //If we are in this deadband, we won't command anythign to the wheels if we are trying to slow down
    const float WHEEL_DEAD_BAND_MM_S = 2; // TODO: float or int?
    
    
    //Set the gains for the PID controllers (same gains for left and right wheel)
    void SetGains(float kp, float ki, float kd);
    
    //This manages at a high level what the wheel speed controller needs to do
    void Manage();
    
    //Sets/Gets the desired speeds for the wheels (in mm/sec forward speed)
    void GetDesiredWheelSpeeds(f32 *leftws, f32 *rightws);
    void SetDesiredWheelSpeeds(f32 leftws, f32 rightws);
    
    void GetFilteredWheelSpeeds(f32 *left, f32 *right);
    
    //This function will command a wheel speed to the left and right wheel so that the vehicle follows a trajectory
    //This will only work if the steering controller does not overwrite the values.
    void utilSetVehicleOLTrajectory( u16 radius, u16 vspeed );
    
    // Whether the wheel controller should be coasting (not actively trying to run
    // wheel controllers
    void SetCoastMode(bool isOn);
    
    
    void ResetIntegralGainSums(void);
    
    
    // Call this to suppress drive to motors until car is stopped.
    // Used to make sure car doesn't keep driving when it delocalizes.
    void DoCoastUntilStop(void);
    
  } // namespace WheelController
  } // namespace Cozmo
} // namespace Anki


#endif
