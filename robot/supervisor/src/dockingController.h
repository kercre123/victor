/**
 * File: dockingController.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 10/29/2013
 *
 * Information on last revision to this file:
 *    $LastChangedDate$
 *    $LastChangedBy$
 *    $LastChangedRevision$
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

#include "anki/cozmo/messageProtocol.h"

namespace Anki {
  namespace Cozmo {
    namespace DockingController {

      // TODO: Add if/when needed?
      // ReturnCode Init();
      
      ReturnCode SetGoals(const CozmoMsg_InitiateDock* msg);
      bool IsDone(); // whether or not it was successful
      bool DidSucceed();
      ReturnCode Update();
      
      // Gripper control
      void EngageGripper();
      void DisengageGripper();
      bool IsGripperEngaged();
      
    } // namespace DockingController
  } // namespace Cozmo
} // namespace Anki

#endif // COZMO_DOCKING_CONTROLLER_H_