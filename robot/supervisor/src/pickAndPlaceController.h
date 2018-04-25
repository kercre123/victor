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

#include "coretech/common/shared/types.h"
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
        MOVING_LIFT_FOR_DEEP_ROLL,
        FACE_PLANTING_BACKOUT,
        FACE_PLANTING,
        POPPING_A_WHEELIE,
        MOVING_LIFT_POSTDOCK,
        BACKOUT,
        TRAVERSE_RAMP,
        TRAVERSE_RAMP_DOWN,
        ENTER_BRIDGE,
        TRAVERSE_BRIDGE,
        LEAVE_BRIDGE,
        PICKUP_ANIM,
        BACKUP_ON_CHARGER,
        DRIVE_FORWARD,
      } Mode;

      Result Init();

      Result Update();

      void SetChargerStripeIsBlack(const bool b);
      
      Mode GetMode();
      
      DockAction GetCurAction();

      bool IsBusy();
      bool IsCarryingBlock();

      void SetCarryState(CarryState state);
      CarryState GetCarryState();

      // Starts the docking process, relying on the relative pose of the marker to be
      // transmitted from cozmo-engine immediately after calling this. (Except for DA_PLACE_LOW_BLIND)
      // If DA_PLACE_LOW_BLIND, rel_* parameters are wrt to current robot pose. Otherwise, rel_* params
      // are ignored.
      void DockToBlock(const DockAction action,
                       const bool doLiftLoadCheck,
                       const f32 speed_mmps,
                       const f32 accel_mmps2,
                       const f32 decel_mmps2,
                       const f32 rel_x = 0,
                       const f32 rel_y = 0,
                       const f32 rel_angle = 0,
                       const u8 numRetries = 2);

      // Places block on ground and backs out.
      void PlaceOnGround(const f32 speed_mmps,
                         const f32 accel_mmps2,
                         const f32 decel_mmps2,
                         const f32 rel_x,
                         const f32 rel_y,
                         const f32 rel_angle);

      // Abort whatever pick or place action we're currently doing
      void Reset();

      void SetRollActionParams(const f32 liftHeight_mm,
                               const f32 driveSpeed_mmps,
                               const f32 driveAccel_mmps2,
                               const u32 driveDuration_ms,
                               const f32 backupDist_mm);
      
    } // namespace PickAndPlaceController
  } // namespace Cozmo
} // namespace Anki

#endif // COZMO_PICK_AND_PLACE_CONTROLLER_H_
