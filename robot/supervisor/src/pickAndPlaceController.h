/**
 * File: pickAndPlaceController.h
 *
 * Author: Kevin Yoon (kevin)
 * Created: 12/23/2013
 *
 * Description:
 *
 *   Controller for picking up or placing blocks.
 *   The block specified should always be at the same height
 *   as the robot since it cannot see it for full docking duration
 *   otherwise.
 *
 * Copyright: Anki, Inc. 2013
 *
 **/

#ifndef COZMO_PICK_AND_PLACE_CONTROLLER_H_
#define COZMO_PICK_AND_PLACE_CONTROLLER_H_

#include "anki/common/types.h"

#include "anki/cozmo/robot/messages.h"

#include "anki/vision/MarkerCodeDefinitions.h"

namespace Anki {
  namespace Cozmo {
    
    // Forward declaration
    namespace VisionSystem {
      class MarkerCode;
    }
    
    namespace PickAndPlaceController {

      typedef enum {
        IDLE,
        SET_LIFT_PREDOCK,
        MOVING_LIFT_PREDOCK,
        DOCKING,
        SET_LIFT_POSTDOCK,
        MOVING_LIFT_POSTDOCK,
        BACKOUT,
        LOWER_LIFT
      } Mode;
      
      // TODO: Add if/when needed?
      // ReturnCode Init();

      ReturnCode Update();
      
      Mode GetMode();
      
      bool IsBusy();

      bool DidLastActionSucceed();

      // level: 0 = block is at same height as robot (i.e. robot floor height)
      //        1 = block is at one block height above robot floor height.
      void PickUpBlock(const Vision::MarkerType blockID, const f32 markerWidth_mm, const u8 level);
      
      void PlaceOnBlock(const Vision::MarkerType blockID,
                        const f32 horizontal_offset, const f32 angular_offset);
      
    } // namespace PickAndPlaceController
  } // namespace Cozmo
} // namespace Anki

#endif // COZMO_PICK_AND_PLACE_CONTROLLER_H_
