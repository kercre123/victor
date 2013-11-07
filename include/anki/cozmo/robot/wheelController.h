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
    
    //The default gains for the wheel speed controller
    const float DEFAULT_WHEEL_KP = 0.1f; //1.0f
    const float DEFAULT_WHEEL_KI = 0.005f; //0.05f
    const float DEFAULT_WHEEL_KD = 0.0f;
    
    const float WHEEL_SPEED_COMMAND_STOPPED_MM_S = 2.0;
    
    //How fast (in mm/sec) can a wheel spin at max
    const float MAX_WHEEL_SPEED_MM_S = 1000; // TODO: float or int?
    const float MIN_WHEEL_SPEED_MM_S = 0;    // TODO: float or int?
    
    //If we drive slower than this, the vehicle is stopped
    const float WHEEL_SPEED_CONSIDER_STOPPED_MM_S = 2; // TODO: float or int?
    
    //If we are in this deadband, we won't command anythign to the wheels if we are trying to slow down
    const float WHEEL_DEAD_BAND_MM_S = 2; // TODO: float or int?
    
    
    // This is an openloop function which is supposed convert mm/sec to motor values.
    // ATTENTION: This is open loop, and by definition naive and not accurate
    // It depends on what surface we drive on, what the other whell does, etc.
    // The ONLY reason for this function is to make it a little easier for the PID controller
    // ALso, if possible, it would be good to come up with a better one by taking data
    // off of a running car and see what speed we get when driving straight at different
    // velocities
    //#if HAS_BELKER_SHELL
#define MM_PER_SEC_TO_MOTOR_VAL(n) (n)
    //#else
    //#define MM_PER_SEC_TO_MOTOR_VAL(n) (n + 500.0f)
    //#endif
    
    //Set the gains for the PID controllers (same gains for left and right wheel)
    void SetGains(float kp, float ki, float kd);
    
    //This manages at a high level what the wheel speed controller needs to do
    void Manage();
    
    //Sets/Gets the desired speeds for the wheels (in mm/sec forward speed)
    void GetDesiredWheelSpeeds(s16 *leftws, s16 *rightws);
    void SetDesiredWheelSpeeds(s16 leftws, s16 rightws);
    
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
