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

#include "anki/types.h"
#include "anki/vision/MarkerCodeDefinitions.h"
#include "anki/common/robot/geometry_declarations.h"
#include "clad/types/dockingSignals.h"

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
        MOVING_LIFT_FOR_ROLL,
        POPPING_A_WHEELIE,
        MOVING_LIFT_POSTDOCK,
        BACKOUT,
        LOWER_LIFT,
        TRAVERSE_RAMP,
        TRAVERSE_RAMP_DOWN,
        ENTER_BRIDGE,
        TRAVERSE_BRIDGE,
        LEAVE_BRIDGE,
        ROTATE_FOR_CHARGER_APPROACH,
        BACKUP_ON_CHARGER
      } Mode;
      
      Result Init();

      Result Update();
      
      Mode GetMode();
      
      bool IsBusy();
      bool IsCarryingBlock();
      bool DidLastActionSucceed();

      void SetCarryState(CarryState state);
      CarryState GetCarryState();
      
      // Starts the docking process, relying on the relative pose of the marker to be
      // transmitted from cozmo-engine immediately after calling this. (Except for DA_PLACE_LOW_BLIND)
      // If DA_PLACE_LOW_BLIND, rel_* parameters are wrt to current robot pose. Otherwise, rel_* params
      // are ignored.
      void DockToBlock(const DockAction action,
                       const f32 rel_x = 0,
                       const f32 rel_y = 0,
                       const f32 rel_angle = 0,
                       const bool useManualSpeed = false);
      
      // Places block on ground and backs out.
      void PlaceOnGround(const f32 rel_x, const f32 rel_y, const f32 rel_angle, const bool useManualSpeed);
      
      // Abort whatever pick or place action we're currently doing
      void Reset();
      
    } // namespace PickAndPlaceController
  } // namespace Cozmo
} // namespace Anki

#endif // COZMO_PICK_AND_PLACE_CONTROLLER_H_
