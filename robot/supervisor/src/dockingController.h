/**
 * File: dockingController.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 10/29/2013
 *
 * Description:
 *
 *   Controller for the docking process (approaching the block and lifting)
 *
 * Copyright: Anki, Inc. 2013
 *
 **/

#ifndef COZMO_DOCKING_CONTROLLER_H_
#define COZMO_DOCKING_CONTROLLER_H_

#include "anki/common/types.h"

#include "anki/cozmo/messages.h"

namespace Anki {
  namespace Cozmo {
    namespace DockingController {

      // TODO: Add if/when needed?
      // ReturnCode Init();
      
      bool IsDocked(); // whether or not it was successful
      bool IsDockingOrPlacing();
      bool DidSucceed();
      ReturnCode Update();

      // Set the desired
      void SetRelDockPose(f32 rel_x, f32 rel_y, f32 rel_rad);
      
      // Resets state machine and configures VisionSystem to track
      // appropriate block
      void ResetDocker();
      
    } // namespace DockingController
  } // namespace Cozmo
} // namespace Anki

#endif // COZMO_DOCKING_CONTROLLER_H_