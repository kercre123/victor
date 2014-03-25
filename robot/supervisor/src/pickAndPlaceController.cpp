#include "anki/common/robot/config.h"
#include "pickAndPlaceController.h"
#include "dockingController.h"
#include "headController.h"
#include "liftController.h"

#include "anki/cozmo/cozmoTypes.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/speedController.h"
#include "anki/cozmo/robot/steeringController.h"
#include "anki/cozmo/robot/visionSystem.h"


namespace Anki {
  namespace Cozmo {
    namespace PickAndPlaceController {
      
      namespace {
        
        // Constants
        
        // TODO: Need to be able to specify wheel motion by distance
        const u32 BACKOUT_TIME = 1500000;
        const f32 BACKOUT_SPEED_MMPS = -30;
        
        // Distance between the robot origin and the distance along the robot's x-axis
        // to the lift when it is in the low docking position.
        const f32 ORIGIN_TO_LOW_LIFT_DIST_MM = 32.f;
        const f32 ORIGIN_TO_HIGH_PLACEMENT_DIST_MM = 25.f;

        Mode mode_ = IDLE;
        
        DockAction_t action_ = DA_PICKUP_LOW;
        
        Vision::MarkerType dockToMarker_;
        f32 markerWidth_ = 0;
        f32 dockOffsetDistX_ = 0;
        f32 dockOffsetDistY_ = 0;
        f32 dockOffsetAng_ = 0;
        
        bool isCarryingBlock_ = false;
        bool lastActionSucceeded_ = false;
        
        // When to transition to the next state. Only some states use this.
        u32 transitionTime_ = 0;
        
      } // "private" namespace
      
      
      Mode GetMode() {
        return mode_;
      }
      
      
      ReturnCode Update()
      {
        ReturnCode retVal = EXIT_SUCCESS;
        
        switch(mode_)
        {
          case IDLE:
            break;
            
          case SET_LIFT_PREDOCK:
          {
#if(DEBUG_PAP_CONTROLLER)
            PRINT("PAP: SETTING LIFT PREDOCK\n");
#endif
            mode_ = MOVING_LIFT_PREDOCK;
            switch(action_) {
              case DA_PICKUP_LOW:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK);
                break;
              case DA_PICKUP_HIGH:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_HIGHDOCK);
                break;
              case DA_PLACE_HIGH:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY);
                break;
              default:
                PRINT("ERROR: Unknown PickAndPlaceAction %d\n", action_);
                mode_ = IDLE;
                break;
            }
            break;
          }

          case MOVING_LIFT_PREDOCK:
            if (LiftController::IsInPosition()) {
              DockingController::StartDocking(dockToMarker_, markerWidth_, dockOffsetDistX_);
              mode_ = DOCKING;
#if(DEBUG_PAP_CONTROLLER)
              PRINT("PAP: DOCKING\n");
#endif
            }
            break;

          case DOCKING:
             
            if (!DockingController::IsBusy()) {

              if (DockingController::DidLastDockSucceed()) {
                // Docking is complete
                mode_ = SET_LIFT_POSTDOCK;
              } else {
                // Block is not being tracked.
                // Probably not visible.
#if(DEBUG_PAP_CONTROLLER)
                PRINT("WARN (PickAndPlaceController): Could not track block's marker\n");
#endif
                // TODO: Send BTLE message notifying failure
                mode_ = IDLE;
              }
            }
            break;

          case SET_LIFT_POSTDOCK:
          {
#if(DEBUG_PAP_CONTROLLER)
            PRINT("PAP: SETTING LIFT POSTDOCK\n");
#endif
            mode_ = MOVING_LIFT_POSTDOCK;
            switch(action_) {
              case DA_PICKUP_LOW:
              case DA_PICKUP_HIGH:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY);
                break;
              case DA_PLACE_HIGH:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_HIGHDOCK);
                break;
              default:
                PRINT("ERROR: Unknown PickAndPlaceAction %d\n", action_);
                mode_ = IDLE;
                break;
            }
            break;
          }
            
          case MOVING_LIFT_POSTDOCK:
            if (LiftController::IsInPosition()) {
              switch(action_) {
                case DA_PICKUP_LOW:
                case DA_PICKUP_HIGH:
                  mode_ = IDLE;
                  lastActionSucceeded_ = true;
                  isCarryingBlock_ = true;
                  break;
                case DA_PLACE_HIGH:
                  SteeringController::ExecuteDirectDrive(BACKOUT_SPEED_MMPS, BACKOUT_SPEED_MMPS);
                  transitionTime_ = HAL::GetMicroCounter() + BACKOUT_TIME;
                  mode_ = BACKOUT;
#if(DEBUG_PAP_CONTROLLER)
                  PRINT("PAP: BACKING OUT\n");
#endif
                  break;
              }
            }
            break;
            
          case BACKOUT:
            if (HAL::GetMicroCounter() > transitionTime_) {
#if(DEBUG_PAP_CONTROLLER)
              PRINT("PAP: LOWERING LIFT\n");
#endif
              SteeringController::ExecuteDirectDrive(0,0);
              LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK);
              mode_ = LOWER_LIFT;
            }
            break;
          case LOWER_LIFT:
            if (LiftController::IsInPosition()) {
#if(DEBUG_PAP_CONTROLLER)
              PRINT("PAP: IDLE\n");
#endif
              mode_ = IDLE;
              lastActionSucceeded_ = true;
              isCarryingBlock_ = false;
            }
            break;
            
          default:
            mode_ = IDLE;
            PRINT("Reached default case in DockingController "
                  "mode switch statement.(1)\n");
            break;
        }
        
        return retVal;
        
      } // Update()

      bool DidLastActionSucceed() {
        return lastActionSucceeded_;
      }

      bool IsBusy()
      {
        return mode_ != IDLE;
      }

      bool IsCarryingBlock()
      {
        return isCarryingBlock_;
      }
                
      void PickUpBlock(const Vision::MarkerType blockMarker, const f32 markerWidth_mm, const u8 level)
      {
        // TODO: If block blockID is on level 1, the robot should first
        // identify the block directly below it and then dock to that
        // block's marker with a horizontal offset that will align
        // the fork with block blockID.
        // ...
#if(DEBUG_PAP_CONTROLLER)
        PRINT("PAP: PICKING UP BLOCK %d\n", blockMarker);
#endif
        if (level == 0) {
          action_ = DA_PICKUP_LOW;
          dockOffsetDistX_ = ORIGIN_TO_LOW_LIFT_DIST_MM;
        } else {
          action_ = DA_PICKUP_HIGH;
          dockOffsetDistX_ = ORIGIN_TO_HIGH_PLACEMENT_DIST_MM;
        }
        
        dockToMarker_ = blockMarker;
        markerWidth_  = markerWidth_mm;
        
        mode_ = SET_LIFT_PREDOCK;
        lastActionSucceeded_ = false;
      }
      
      void PlaceOnBlock(const Vision::MarkerType blockMarker,
                        const f32 horizontal_offset, const f32 angular_offset)
      {
        // TODO: Confirm that blockID is on level 0?
        
        // TODO: Pass along docking offset to DockingController
#if(DEBUG_PAP_CONTROLLER)
        PRINT("PAP: PLACING BLOCK on %d\n", blockMarker);
#endif
        action_ = DA_PLACE_HIGH;
        dockOffsetDistX_ = ORIGIN_TO_HIGH_PLACEMENT_DIST_MM;
        dockToMarker_ = blockMarker;
        mode_ = SET_LIFT_PREDOCK;
        lastActionSucceeded_ = false;        
      }
      
      
    } // namespace PickAndPlaceController
  } // namespace Cozmo
} // namespace Anki
