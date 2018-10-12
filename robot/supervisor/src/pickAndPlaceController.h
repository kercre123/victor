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
  namespace Vector {

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
        PICKUP_ANIM,
        BACKUP_ON_CHARGER,
        DRIVE_FORWARD,
        CLIFF_ALIGN_TO_WHITE,
      } Mode;

      Result Init();

      Result Update();
      
      Mode GetMode();
      
      DockAction GetCurAction();

      bool IsBusy();
      bool IsCarryingBlock();
      bool IsPickingUp();

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

      // Start and stop action to align cliff sensors to "white" line
      // One front sensor must already be detecting white when this is called.
      void CliffAlignToWhite();
      void StopCliffAlignToWhite();

      // Abort whatever pick or place action we're currently doing
      void Reset();

      void SetRollActionParams(const f32 liftHeight_mm,
                               const f32 driveSpeed_mmps,
                               const f32 driveAccel_mmps2,
                               const u32 driveDuration_ms,
                               const f32 backupDist_mm);
      
      // When set, this will cause the robot to begin backing up at the same time that he begins to raise the lift
      // during cube pickup. This prevents cube pickup from failing when the cube is pressed up against a wall or other
      // rigid surface.
      void SetBackUpWhileLiftingCube(const bool b);
      
    } // namespace PickAndPlaceController
  } // namespace Vector
} // namespace Anki

#endif // COZMO_PICK_AND_PLACE_CONTROLLER_H_
