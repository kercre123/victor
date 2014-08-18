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
#include "anki/vision/MarkerCodeDefinitions.h"
#include "anki/common/robot/geometry_declarations.h"
#include "anki/cozmo/shared/cozmoTypes.h"

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
        LOWER_LIFT,
        TRAVERSE_RAMP,
        TRAVERSE_RAMP_DOWN,
        ENTER_BRIDGE,
        TRAVERSE_BRIDGE,
        LEAVE_BRIDGE
      } Mode;
      
      Result Init();

      Result Update();
      
      Mode GetMode();
      
      bool IsBusy();
      bool IsCarryingBlock();
      bool DidLastActionSucceed();

      
      // Picks up the specified block, or places the block in hand on top of the specified block
      // depending on the specified action.
      void DockToBlock(const Vision::MarkerType markerType,
                       const Vision::MarkerType markerType2,
                       const f32 markerWidth_mm,
                       const DockAction_t action);
      
      // Same as above except docking will only occur if the specified marker is found
      // at the specified image coordinates within pixelSearchRadius
      void DockToBlock(const Vision::MarkerType markerType,
                       const Vision::MarkerType markerType2,
                       const f32 markerWidth_mm,
                       const Embedded::Point2f& markerCenter,
                       const f32 pixelSearchRadius,
                       const DockAction_t action);
       
      // Places block on ground and backs out.
      void PlaceOnGround(const f32 rel_x, const f32 rel_y, const f32 rel_angle);
      
      // Abort whatever pick or place action we're currently doing
      void Reset();
      
    } // namespace PickAndPlaceController
  } // namespace Cozmo
} // namespace Anki

#endif // COZMO_PICK_AND_PLACE_CONTROLLER_H_
