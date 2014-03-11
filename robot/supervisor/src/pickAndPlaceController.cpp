#include "anki/common/robot/config.h"
#include "pickAndPlaceController.h"
#include "dockingController.h"
#include "headController.h"
#include "liftController.h"

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
        enum Action {
          DOCKING_LOW,   // Docking to block at level 0
          DOCKING_HIGH,  // Docking to block at level 1
          PLACING_HIGH   // Placing block atop another block at level 0
        };
        
        // HACK: Only using these because lift controller encoders not working.
        const u32 PRE_PLACEMENT_WAIT_TIME_US = 2000000;
        const u32 LIFT_MOTION_TIME_US = 5000000;
        const u32 LIFT_PLACEMENT_ADJUST_TIME_US = 150000;
        const u32 BACKOUT_TIME = 3000000;
        const f32 BACKOUT_SPEED_MMPS = -30;
        
        // Distance between the robot origin and the distance along the robot's x-axis
        // to the lift when it is in the low docking position.
        const f32 ORIGIN_TO_LOW_LIFT_DIST_M = 0.035;
        const f32 ORIGIN_TO_HIGH_PLACEMENT_DIST_M = 0.0;

        Mode mode_ = IDLE;
        Action action_ = DOCKING_LOW;
        
        VisionSystem::MarkerCode dockToMarkerCode_;
        f32 dockOffsetDistX_ = 0;
        f32 dockOffsetDistY_ = 0;
        f32 dockOffsetAng_ = 0;
        
        
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
            PRINT("PAP: SETTING LIFT PREDOCK\n");
            mode_ = MOVING_LIFT_PREDOCK;
            switch(action_) {
              case DOCKING_LOW:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK);
                break;
              case DOCKING_HIGH:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_HIGHDOCK);
                break;
              case PLACING_HIGH:
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
              DockingController::StartDocking(dockToMarkerCode_, dockOffsetDistX_);
              mode_ = DOCKING;
              PRINT("PAP: DOCKING\n");
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
                PRINT("WARN (PickAndPlaceController): Could not track block's marker\n");
                // TODO: Send BTLE message notifying failure
                mode_ = IDLE;
              }
            }
            break;

          case SET_LIFT_POSTDOCK:
          {
            PRINT("PAP: SETTING LIFT POSTDOCK\n");
            mode_ = MOVING_LIFT_POSTDOCK;
            switch(action_) {
              case DOCKING_LOW:
              case DOCKING_HIGH:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY);
                break;
              case PLACING_HIGH:
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
                case DOCKING_LOW:
                case DOCKING_HIGH:
                  mode_ = IDLE;
                  lastActionSucceeded_ = true;
                  break;
                case PLACING_HIGH:
                  SteeringController::ExecuteDirectDrive(BACKOUT_SPEED_MMPS, BACKOUT_SPEED_MMPS);
                  transitionTime_ = HAL::GetMicroCounter() + BACKOUT_TIME;
                  mode_ = BACKOUT;
                  PRINT("PAP: BACKING OUT\n");
                  break;
              }
            }
            break;
            
          case BACKOUT:
            if (HAL::GetMicroCounter() > transitionTime_) {
              PRINT("PAP: LOWERING LIFT\n");
              SteeringController::ExecuteDirectDrive(0,0);
              HAL::MotorSetPower(HAL::MOTOR_LIFT, -0.3);
              transitionTime_ = HAL::GetMicroCounter() + LIFT_MOTION_TIME_US;
              mode_ = LOWER_LIFT;
            }
            break;
          case LOWER_LIFT:
            if (HAL::GetMicroCounter() > transitionTime_) {
              PRINT("PAP: IDLE\n");
              HAL::MotorSetPower(HAL::MOTOR_LIFT, 0);
              //SpeedController::SetUserCommandedDesiredVehicleSpeed(0);
              mode_ = IDLE;
              lastActionSucceeded_ = true;
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
                
      void PickUpBlock(const VisionSystem::MarkerCode& blockMarkerCode, const u8 level)
      {
        // TODO: If block blockID is on level 1, the robot should first
        // identify the block directly below it and then dock to that
        // block's marker with a horizontal offset that will align
        // the fork with block blockID.
        // ...

        PRINT("PAP: PICKING UP BLOCK\n");
        
        if (level == 0) {
          action_ = DOCKING_LOW;
          dockOffsetDistX_ = ORIGIN_TO_LOW_LIFT_DIST_M;
        } else {
          action_ = DOCKING_HIGH;
          dockOffsetDistX_ = ORIGIN_TO_HIGH_PLACEMENT_DIST_M;
        }
        
        dockToMarkerCode_.Set(blockMarkerCode);
        
        mode_ = SET_LIFT_PREDOCK;
        lastActionSucceeded_ = false;
      }
      
      void PlaceOnBlock(const VisionSystem::MarkerCode& blockMarkerCode,
                        const f32 horizontal_offset, const f32 angular_offset)
      {
        // TODO: Confirm that blockID is on level 0?
        
        // TODO: Pass along docking offset to DockingController

        PRINT("PAP: PLACING BLOCK\n");
        
        action_ = PLACING_HIGH;
        dockToMarkerCode_.Set(blockMarkerCode);
        mode_ = SET_LIFT_PREDOCK;
        lastActionSucceeded_ = false;        
      }
      
      
    } // namespace PickAndPlaceController
  } // namespace Cozmo
} // namespace Anki
