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


namespace Anki {
  
  namespace Cozmo {
    
    namespace DockingController {

      // TODO: Add if/when needed?
      // Result Init();
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
      void SetRelDockPose(f32 rel_x, f32 rel_y, f32 rel_rad, TimeStamp_t t = HAL::GetTimeStamp());
      
      // Resets state machine and configures VisionSystem to track
      // appropriate block
      void ResetDocker();
     
      // Initiates the vision system to look for block with specified ID. Once found, the
      // robot will follow a docking path to the block. If the block is lost for more than 1 sec,
      // it gives up.
      // dockOffsetDistX: Distance along normal from block face that the robot should "dock" to.
      // dockOffsetDistY: Horizontal offset from block face center that the robot should "dock" to.
      //                  +ve = left of robot.
      //                  e.g. To place a block on top of two other blocks, the robot would need to "dock" to
      //                       one of the blocks at some horizontal offset.
      // dockOffsetAngle: Docking offset angle. +ve means block is facing robot's right side.
      void StartDocking(const Vision::MarkerType& codeToDockWith,
                        const f32 markerWidth_mm,
                        const f32 dockOffsetDistX, const f32 dockOffsetDistY = 0, const f32 dockOffsetAngle = 0,
                        const bool checkAngleX = true,
                        const bool useManualSpeed = false,
                        const u32 pointOfNoReturnDistMM = 0);
      
      // Same as above except the marker must be found within the image at the specified location.
      // If pixel_radius == u8_MAX, the location is ignored and this function becomes identical
      // to the above function.
      void StartDocking(const Vision::MarkerType& codeToDockWith,
                        const f32 markerWidth_mm,
                        const Embedded::Point2f &markerCenter, const u8 pixel_radius,
                        const f32 dockOffsetDistX, const f32 dockOffsetDistY = 0, const f32 dockOffsetAngle = 0,
                        const bool checkAngleX = true,
                        const bool useManualSpeed = false,
                        const u32 pointOfNoReturnDistMM = 0);

      // Goes to a pose such that if the robot were to lower a block that it was carrying once it
      // were in that pose, the block face facing the robot would be aligned with the pose specified
      // relative to the pose of the robot at the time "docking" started.
      // No vision markers are required as this is a "blind docking" maneuver.
      void StartDockingToRelPose(const f32 rel_x, const f32 rel_y, const f32 rel_angle, const bool useManualSpeed = false);
      
      // Keep lift crossbar just below the camera's field of view.
      // Required for docking to high blocks.
      void TrackCamWithLift(bool on);

      // Start tracking the specified marker but don't try to dock with it
      void StartTrackingOnly(const Vision::MarkerType& trackingMarker,
                             const f32 markerWidth_mm);

      // If a marker pose was received from VisionSystem,
      // returns true along with that pose.
      bool GetLastMarkerPose(f32 &x, f32 &y, f32 &angle);
      
    } // namespace DockingController
  } // namespace Cozmo
} // namespace Anki

#endif // COZMO_DOCKING_CONTROLLER_H_
