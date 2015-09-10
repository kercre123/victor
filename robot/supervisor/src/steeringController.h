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

#include "anki/cozmo/shared/cozmoConfig.h"

// TODO: Note that the steering controller is not necessarily cozmo-specific!
//       (So I didn't put it in a "Cozmo" and it coudl potentially be moved
//        out of products-cozmo)

namespace Anki {
  namespace Cozmo {
  namespace SteeringController {
    
    // Available steering controller modes.
    // Manage() must be called regularly regardless of which mode the controller is in.
    // The default mode is SM_PATH_FOLLOW which uses the fidx passed in via Manage to determine steering.
    // The other modes have a one-shot behavior and fall back to SM_PATH_FOLLOW when they complete.
    typedef enum {
      // This is the default mode.
      // Uses fidx to command steering controller. Respects user commanded speed.
      SM_PATH_FOLLOW,
      
      // Wheel speeds are commanded directly.
      // See ExecuteDirectDrive().
      SM_DIRECT_DRIVE,
      
      // For executing point turn.
      // See ExecutePointTurn().
      SM_POINT_TURN
    } SteerMode;
    
    
    // Parameters / Constants:
    const float M_PER_MM = 0.001f;  /**< Meters per millimeter */
    
    // TODO: These constants are Drive-specific and should be provided at Init()
    
    //The gains for the steering controller
    //Heading tracking gain K1, Crosstrack approach rate K2
    const float DEFAULT_STEERING_K1 = 0.12f;
    const float DEFAULT_STEERING_K2 = 12.f; //5.0f; //2.0f

    // Set maximum rotation speed
    void SetRotationSpeedLimit(f32 rad_per_s);
    void DisableRotationSpeedLimit();
    
    //This manages at a high level what the steering controller needs to do (steer, use open loop, etc.)
    void Manage();
    
    SteerMode GetMode();
    
    // SM_PATH_FOLLOW
    // Simply sets the mode to path following mode which on every cycle
    // tries to reduce the current path error. Respects the speed as set by
    // SpeedController::SetUserCommandedDesiredVehicleSpeed().
    void SetPathFollowMode();
    
    // SM_DIRECT_DRIVE
    // TODO(kevin): Command left and right wheel speeds and accelerations.
    //               Terminate with stop function or terminating condition?
    // left_vel: Target left wheel speed in mm/s
    // right_vel: Target right wheel speed in mm/s
    // left_accel: Acceleration with which to approach left_vel in mm/s^2. (Sign is ignored.)
    // right_accel: Acceleration with which to approach right_vel in mm/s^2. (Sign is ignored.)
    void ExecuteDirectDrive(f32 left_vel, f32 right_vel, f32 left_accel = 0, f32 right_accel = 0);
    
    // SM_POINT_TURN
    // Executes a point turn to the target angle.
    // targetAngle: Target robot orientation in radians in frame of current mat.
    // maxAngularVel: Maximum angular speed in rad/s
    // angularAccel: Angular acceleration in rad/s^2
    // angularDecel: Angular decelration in rad/s^2
    // (Sets user commanded speed to 0 if it is not already.)
    // useShortestDir: If true, ignores the sign of maxAngularVel and takes the shortest path to targetAngle.
    void ExecutePointTurn(f32 targetAngle, f32 maxAngularVel, f32 angularAccel, f32 angularDecel, bool useShortestDir);
    
    // Same as above except that it doesn't stop turning. (i.e. no target angle)
    void ExecutePointTurn(f32 angularVel, f32 angularAccel);
    
    
    
    //Steering controllers PD constants and the window size of the derivative
    void SetGains(float k1, float k2);
    
    // Function to init steering controller when we expect to feed it a discontinuous
    // follow line index so that it doesn't compare it to the previous follow line index.
    void ReInit(void);
    
  } // namespace SteeringController
  } // namespace Cozmo
} // namespace Anki

#endif // STEERING_CONTROLLER_H_
