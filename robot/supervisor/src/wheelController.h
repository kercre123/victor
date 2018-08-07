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
  namespace Vector {
  namespace WheelController {
    
    const float WHEEL_SPEED_COMMAND_STOPPED_MM_S = 2.0;
    
    //If we drive slower than this, the vehicle is stopped
    const float WHEEL_SPEED_CONSIDER_STOPPED_MM_S = 2; // TODO: float or int?
    
    //If we are in this deadband, we won't command anythign to the wheels if we are trying to slow down
    const float WHEEL_DEAD_BAND_MM_S = 2; // TODO: float or int?
    
    
    //Set the gains for the PI controllers (same gains for left and right wheel)
    void SetGains(const f32 kp, const f32 ki, const f32 maxIntegralError);

    // Enable/Disable the wheel controller.
    // Mostly only useful for test mode or special simulation modes.
    void Enable();
    void Disable();
    
    //This manages at a high level what the wheel speed controller needs to do
    void Manage();
    
    //Sets/Gets the desired speeds for the wheels (in mm/sec forward speed)
    void GetDesiredWheelSpeeds(f32 &leftws, f32 &rightws);
    void SetDesiredWheelSpeeds(f32 leftws, f32 rightws);
    
    void GetFilteredWheelSpeeds(f32 &left, f32 &right);
    f32 GetAverageFilteredWheelSpeed();
    
    // Returns the maximum possible rotation speed given the current wheel speeds.
    // This is an upper bound on the actual rotation speed since with treads there is significant slip.
    f32 GetCurrNoSlipBodyRotSpeed();
    
    //This function will command a wheel speed to the left and right wheel so that the vehicle follows a trajectory
    //This will only work if the steering controller does not overwrite the values.
    void utilSetVehicleOLTrajectory( u16 radius, u16 vspeed );
    
    // Whether the wheel controller should be coasting (not actively trying to run
    // wheel controllers
    void SetCoastMode(bool isOn);
    
    bool AreWheelsPowered();
    bool AreWheelsMoving();
    
    void ResetIntegralGainSums(void);
    
    
    // Call this to suppress drive to motors until car is stopped.
    // Used to make sure car doesn't keep driving when it delocalizes.
    void DoCoastUntilStop(void);
    
  } // namespace WheelController
  } // namespace Vector
} // namespace Anki


#endif
