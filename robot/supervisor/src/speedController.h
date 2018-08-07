/**
 * File: speedController.h
 *
 **/

#ifndef VEHICLEARM_SPEED_CONTROLLER_H_
#define VEHICLEARM_SPEED_CONTROLLER_H_

namespace Anki {
  namespace Vector {
  namespace SpeedController {
    
    // Controller params
    const float KP = 2.f;
    const float KI = 0.01f;
    const float MAX_ERRORSUM = 20000.f;
    
    //If we drive slower than this, then we consider the car to be stopped
    const float SPEED_CONSIDER_VEHICLE_STOPPED_MM_S = 2.f;
    
    
    // Sets/Gets the speed the user wants the vehicle to drive at (eventually, after acceleration phase)
    void SetUserCommandedDesiredVehicleSpeed(s16 ucspeed);
    s16 GetUserCommandedDesiredVehicleSpeed(void);
    
    // Set/Get the current target speed, while we are ramping up according to the acceleration profile
    void SetUserCommandedCurrentVehicleSpeed(s16 ucspeed);
    s16 GetUserCommandedCurrentVehicleSpeed(void);
    
    // Set both the desired and current vehicle speed simultaneously.
    // This is for special case handling when we want the vehicle to reach the desired speed ASAP
    // Ignores acceleration completely, and just sets the userCommandedCurrentVehicleSpeed
    void SetBothDesiredAndCurrentUserSpeed(s16 ucspeed);
    
    // Set/Get the userCommanded Acceleration
    void SetUserCommandedAcceleration(u16 ucAccel);
    u16 GetUserCommandedAcceleration(void);

    void SetUserCommandedDeceleration(u16 ucDecel);
    u16 GetUserCommandedDeceleration(void);
    
    void SetDefaultAccelerationAndDeceleration();
    
    // The speed controller will vary the speed around the user commanded speed
    // based on how fast we actually drive
    void SetControllerCommandedVehicleSpeed(s16 ccspeed);
    s16 GetControllerCommandedVehicleSpeed(void);
    
    // This tells us how fast the vehicle is driving right now in mm/sec
    s16 GetCurrentMeasuredVehicleSpeed(void);
    
    // Update userCommandedCurrentVehicleSpeed with the userCommandedAcceleration,
    // so it would approach userCommandedDesiredVehicleSpeed
    // This function assumes that userCommandedAcceleration is ALWAYS a positive value
    void RunAccelerationUpdate(void);
    
    //Runs one iteration of the vheicle speed controller to compute the commanded vehicle speed
    void Manage(void);
    
  } // namespace SpeedController
  } // namespace Vector
} // namespace Anki

#endif
