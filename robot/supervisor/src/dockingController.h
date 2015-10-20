/**
 * File: dockingController.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 10/29/2013
 *
 * Description:
 *
 *   Controller for docking to a block, where 'docking' does not necessarily mean
 *   the forklift is engaging the block but rather following some sort of approach
 *   path based on a specific block's relative location.
 *
 * Copyright: Anki, Inc. 2013
 *
 **/

#ifndef COZMO_DOCKING_CONTROLLER_H_
#define COZMO_DOCKING_CONTROLLER_H_

#include "anki/common/types.h"
#include "anki/vision/MarkerCodeDefinitions.h"
#include "anki/common/robot/geometry_declarations.h"
#include "clad/types/dockingSignals.h"


// Distance between the robot origin and the distance along the robot's x-axis
// to the lift when it is in the low docking position.
#ifdef SIMULATOR
/*
const f32 ORIGIN_TO_LOW_LIFT_DIST_MM = 24.f;
const f32 ORIGIN_TO_HIGH_LIFT_DIST_MM = 14.f;
const f32 ORIGIN_TO_HIGH_PLACEMENT_DIST_MM = 12.f;
const f32 ORIGIN_TO_LOW_ROLL_DIST_MM = 14.f;
*/
const f32 ORIGIN_TO_LOW_LIFT_DIST_MM = 28.f;
const f32 ORIGIN_TO_HIGH_LIFT_DIST_MM = 20.f;
const f32 ORIGIN_TO_HIGH_PLACEMENT_DIST_MM = 19.f;
const f32 ORIGIN_TO_LOW_ROLL_DIST_MM = 16.f;

#else
// Cozmo v4.1
const f32 ORIGIN_TO_LOW_LIFT_DIST_MM = 22.f;
const f32 ORIGIN_TO_HIGH_LIFT_DIST_MM = 17.f;
const f32 ORIGIN_TO_HIGH_PLACEMENT_DIST_MM = 20.f;
const f32 ORIGIN_TO_LOW_ROLL_DIST_MM = 13.f;
#endif



namespace Anki {
  
  namespace Cozmo {
    
    namespace DockingController {

      // TODO: Add if/when needed?
      Result Init();
      
      Result Update();
      
      // Returns true if robot is the process of looking for a block or docking to a block
      bool IsBusy();
      
      // Returns true if last attempt to a block succeeded
      bool DidLastDockSucceed();

      // Tells the docker what the relative position of the block is.
      // rel_x: Distance to center of block along robot's x-axis
      // rel_y: Distance to center of block along robot's y-axis
      // rel_rad: Angle of block normal relative to robot's x-axis.
      // t: Timestamp of the pose to which these relative errors should be applied
      //    in order to compute the absolute pose of the target at the current time.
      //    If t == 0 or HAL::GetTimeStamp(), it will use the robot's current pose.
      void SetRelDockPose(f32 rel_x, f32 rel_y, f32 rel_rad, TimeStamp_t t = 0);
      
      // Resets state machine and configures VisionSystem to track
      // appropriate block
      void ResetDocker();
     
      // The robot will follow a docking path generated from an error signal to a marker
      // that it expects to be receiving from cozmo-engine immediately after this is called.
      // dockOffsetDistX: Distance along normal from block face that the robot should "dock" to.
      // dockOffsetDistY: Horizontal offset from block face center that the robot should "dock" to.
      //                  +ve = left of robot.
      //                  e.g. To place a block on top of two other blocks, the robot would need to "dock" to
      //                       one of the blocks at some horizontal offset.
      // dockOffsetAngle: Docking offset angle. +ve means block is facing robot's right side.
      void StartDocking(const f32 dockOffsetDistX, const f32 dockOffsetDistY = 0, const f32 dockOffsetAngle = 0,
                        const bool useManualSpeed = false,
                        const u32 pointOfNoReturnDistMM = 0);
      
      // Goes to a pose such that if the robot were to lower a block that it was carrying once it
      // were in that pose, the block face facing the robot would be aligned with the pose specified
      // relative to the pose of the robot at the time "docking" started.
      // No vision markers are required as this is a "blind docking" maneuver.
      void StartDockingToRelPose(const f32 rel_x, const f32 rel_y, const f32 rel_angle, const bool useManualSpeed = false);

      // If a marker pose was received from VisionSystem,
      // returns true along with that pose.
      bool GetLastMarkerPose(f32 &x, f32 &y, f32 &angle);
      
      // Sets the latest docking error signal message coming from engine
      void SetDockingErrorSignalMessage(const DockingErrorSignal& msg);
      
    } // namespace DockingController
  } // namespace Cozmo
} // namespace Anki

#endif // COZMO_DOCKING_CONTROLLER_H_
