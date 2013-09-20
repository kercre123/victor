/**
 * File: steeringController.h
 *
 * Author: Hanns Tappeiner (hanns)
 * Created: 29-FEB-2009
 * 
 *
 **/

#ifndef STEERING_CONTROLLER_H_
#define STEERING_CONTROLLER_H_

#include "cozmoTypes.h"


#define MIN_STEERING_K2 2.0f
#define MAX_STEERING_K2 20.0f
#define NUM_CYCLES_TO_SPAN_STEERING_K2_RANGE 250


// Enable steering while not commanding speed to motors
// for the purposes of encouraging it to stay on the line when
// it's stopped and being pushed by other cars.
void EnableAlwaysOnSteering(BOOL on);

//The gains for the steering controller
//Heading tracking gain K1, Crosstrack approach rate K2
#define DEFAULT_STEERING_K1 0.001f
#define DEFAULT_STEERING_K2 2.0f

//The distance of the rear wheels (center to center)
#define WHEEL_DIST_MM 38.7f
#define WHEEL_DIST_HALF_MM (WHEEL_DIST_MM/2.0f)

//Non linear version of the steering controller
void RunLineFollowControllerNL(s16 location_pix);

//Steering controllers PD constants and the window size of the derivative
void SetSteeringControllerGains(float k1, float k2);

//This manages at a high level what the steering controller needs to do (steer, use open loop, etc.)
void ManageSteeringController(s16 fidx);

// Resets steering controller gain back to "loosest" setting.
// For dynamic steering gain adjust.
void ResetSteeringGains(void);

// Make steering controller aggressive, bypass normal gradual ramping up.
void SetAggressiveSteeringGains(void);

// Function to init steering controller when we expect to feed it a discontinuous
// follow line index so that it doesn't compare it to the previous follow line index.
void ReInitSteeringController(void);

#endif
