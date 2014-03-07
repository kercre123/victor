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

#include "anki/cozmo/robot/messages.h"

// Just to get VisionMarkerType enum?
#include "anki/vision/robot/visionMarkerDecisionTrees.h"

namespace Anki {
  
  namespace Cozmo {
    
    namespace DockingController {

      // TODO: Add if/when needed?
      // ReturnCode Init();
      ReturnCode Update();
      
      // Returns true if robot is the process of looking for a block or docking to a block
      bool IsBusy();
      
      // Returns true if last attempt to a block succeeded
      bool DidLastDockSucceed();

      // Tells the docker what the relative position of the block is.
      // TODO: Move this to private namespace. Currently, this is only used in one of the test modes.
      void SetRelDockPose(f32 rel_x, f32 rel_y, f32 rel_rad);
      
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
      void StartDocking(const Embedded::VisionMarkerType& codeToDockWith,
                        f32 dockOffsetDistX, f32 dockOffsetDistY = 0, f32 dockOffsetAngle = 0);
      
    } // namespace DockingController
  } // namespace Cozmo
} // namespace Anki

#endif // COZMO_DOCKING_CONTROLLER_H_
