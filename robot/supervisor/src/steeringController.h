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

#include "coretech/common/shared/types.h"

// TODO: Note that the steering controller is not necessarily cozmo-specific!
//       (So I didn't put it in a "Cozmo" and it coudl potentially be moved
//        out of products-cozmo)

namespace Anki {
  namespace Vector {
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

    // Set maximum rotation speed
    void SetRotationSpeedLimit(f32 rad_per_s);
    f32  GetRotationSpeedLimit();
    void DisableRotationSpeedLimit();

    // For Enabling/Disabling all Execute****() functions
    void Enable();
    void Disable();

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
    // numHalfRevolutions: The number of half-rotations the robot should perform. Used for relative turns of
    //   angles greater than 180 degrees, for example. Ignored if useShortestDir is true.
    void ExecutePointTurn(f32 targetAngle, f32 maxAngularVel, f32 angularAccel, f32 angularDecel, f32 angleTolerance, bool useShortestDir = true, uint16_t numHalfRevolutions = 0);
    
    // Same as above except that it doesn't stop turning. (i.e. no target angle)
    void ExecutePointTurn(f32 angularVel, f32 angularAccel);
    
    //Steering controllers gains and caps for path offsets when docking
    //(since cozmo can be fairly far off path when docking)
    void SetGains(f32 k1, f32 k2, f32 dockPathDistOffsetCap_mm, f32 dockPathAngularOffsetCap_rad);
    f32 GetK1Gain();
    f32 GetK2Gain();
    f32 GetPathDistOffsetCap();
    f32 GetPathAngOffsetCap();
    
    // Set gains for point turning
    void SetPointTurnGains(f32 kp, f32 ki, f32 kd, f32 maxIntegralError);
    
    // Function to init steering controller when we expect to feed it a discontinuous
    // follow line index so that it doesn't compare it to the previous follow line index.
    void ReInit(void);
    
    // curvatureRadius_mm: Radius of arc to drive.
    //                     u16_MAX and u16_MIN == Straight.
    //                     0 == point turn.
    //                     +ve: curves left, -ve: curves right
    // speed:              Target speed in mm/sec
    //                     If curvatureRadius_mm == 0, the speed is in rad/s where +ve means CCW rotation.
    // accel:              Acceleration to approach target speed in mm/sec^2 (or rad/s^2 if curvatureRadius_mm == 0)
    //                     0 == Max acceleration
    void ExecuteDriveCurvature(f32 speed, f32 curvatureRadius_mm, f32 accel = 0.f);

    // Saves the current heading
    void RecordHeading();
    
    // Execute point turn to the heading recorded by RecordHeading() + offset_rad
    void ExecutePointTurnToRecordedHeading(f32 offset_rad,
                                           f32 maxAngularVel_radPerSec,
                                           f32 angularAccel_radPerSec2,
                                           f32 angularDecel_radPerSec2,
                                           f32 angleTolerance_rad,
                                           uint16_t numHalfRevolutions,
                                           bool useShortestDir);
    
  } // namespace SteeringController
  } // namespace Vector
} // namespace Anki

#endif // STEERING_CONTROLLER_H_
