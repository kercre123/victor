/**
 * File: vehicleSpeedController.h
 *
 **/

#ifndef VEHICLEARM_SPEED_CONTROLLER_H_
#define VEHICLEARM_SPEED_CONTROLLER_H_

#include "anki/cozmo/robot/cozmoTypes.h"

namespace Anki {
  namespace VehicleSpeedController {
    
    
    // Controller params
#define VEHICLE_SPEED_CONTROLLER_KP 2
#define VEHICLE_SPEED_CONTROLLER_KI 0.01f
#define VEHICLE_SPEED_CONTROLLER_MAX_ERRORSUM 20000
    
    
    //If we drive slower than this, then we consider the car to be stopped
#define SPEED_CONSIDER_VEHICLE_STOPPED_MM_S 2
    
    
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
    void SetUserCommandedAcceleration(s16 ucAccel);
    s16 GetUserCommandedAcceleration(void);
    
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
    void ManageVehicleSpeedController(void);
    
    //Figures out whether the car is stopped or not
    bool IsVehicleStopped(void);
    
    void ResetVehicleSpeedControllerIntegralError(void);
    
    void TraceSpeedControllerVars(u8 traceLevel);
    
  } // namespace VehicleSpeedController
} // namespace Anki

#endif
