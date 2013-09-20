/**
 * File: wheelController.h
 *
 * Author: Hanns Tappeiner (hanns)
 * 
 *
 **/

#ifndef WHEEL_CONTROLLER_H_
#define WHEEL_CONTROLLER_H_

#include "cozmoTypes.h"

//The default gains for the wheel speed controller
#define DEFAULT_WHEEL_KP 1.0f
#define DEFAULT_WHEEL_KI 0.05f
#define DEFAULT_WHEEL_KD 0.0f

#define WHEEL_SPEED_COMMAND_STOPPED_MM_S  2.0

//How fast (in mm/sec) can a wheel spin at max
#define MAX_WHEEL_SPEED_MM_S 1000
#define MIN_WHEEL_SPEED_MM_S 0

//If we drive slower than this, the vehicle is stopped
#define WHEEL_SPEED_CONSIDER_STOPPED_MM_S 2

//If we are in this deadband, we won't command anythign to the wheels if we are trying to slow down
#define WHEEL_DEAD_BAND_MM_S 2


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
void SetWheelSpeedControllerGains(float kp, float ki, float kd);

//This manages at a high level what the wheel speed controller needs to do
void ManageWheelSpeedController(s16 *motorvalueoutL, s16 *motorvalueoutR);

//Sets/Gets the desired speeds for the wheels (in mm/sec forward speed)
void GetDesiredWheelSpeeds(s16 *leftws, s16 *rightws);
void SetDesiredWheelSpeeds(s16 leftws, s16 rightws);

//This function will command a wheel speed to the left and right wheel so that the vehicle follows a trajectory
//This will only work if the steering controller does not overwrite the values.
void utilSetVehicleOLTrajectory( u16 radius, u16 vspeed );

// Whether the wheel controller should be coasting (not actively trying to run
// wheel controllers
void SetWheelControllerCoastMode(const BOOL isOn);


void ResetWheelControllerIntegralGainSums(void);


// Call this to suppress drive to motors until car is stopped.
// Used to make sure car doesn't keep driving when it delocalizes.
void DoCoastUntilStop(void);

#endif
