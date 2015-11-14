#include "anki/common/robot/config.h"
#include "pickAndPlaceController.h"
#include "dockingController.h"
#include "headController.h"
#include "liftController.h"
#include "localization.h"
#include "imuFilter.h"
#include "proxSensors.h"

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "messages.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "clad/types/dockingSignals.h"
#include "speedController.h"
#include "steeringController.h"


#define DEBUG_PAP_CONTROLLER 0

// If you enable this, make sure image streaming is off
// and you enable the print-to-file code in the ImageChunk message handler
// on the basestation.
#define SEND_PICKUP_VERIFICATION_SNAPSHOTS 0


namespace Anki {
  namespace Cozmo {
    namespace PickAndPlaceController {
      
      namespace {
        
        // Constants
        
        // TODO: Need to be able to specify wheel motion by distance
        //const u32 BACKOUT_TIME = 1500000;
        //

        // The distance from the last-observed position of the target that we'd
        // like to be after backing out
        const f32 BACKOUT_DISTANCE_MM = 75.f;
        const f32 BACKOUT_SPEED_MMPS = 60;
        
        const f32 RAMP_TRAVERSE_SPEED_MMPS = 40;
        const f32 ON_RAMP_ANGLE_THRESH = 0.15;
        const f32 OFF_RAMP_ANGLE_THRESH = 0.05;
        
        const f32 BRIDGE_TRAVERSE_SPEED_MMPS = 40;
        
        // Distance at which robot should start driving blind
        // along last generated docking path during DA_PICKUP_HIGH.
        const u32 HIGH_DOCK_POINT_OF_NO_RETURN_DIST_MM = ORIGIN_TO_HIGH_LIFT_DIST_MM + 30;
        
        // Distance at which robot should start driving blind
        // along last generated docking path during PICKUP_LOW and PLACE_HIGH.
        const u32 LOW_DOCK_POINT_OF_NO_RETURN_DIST_MM = ORIGIN_TO_LOW_LIFT_DIST_MM + 20;

        const f32 DEFAULT_LIFT_SPEED_RAD_PER_SEC = 1.5;
        const f32 DEFAULT_LIFT_ACCEL_RAD_PER_SEC2 = 10;
        
        Mode mode_ = IDLE;
        
        DockAction action_ = DA_PICKUP_LOW;

        Embedded::Point2f ptStamp_;
        Radians angleStamp_;
        
        f32 dockOffsetDistX_ = 0;
        f32 dockOffsetDistY_ = 0;
        f32 dockOffsetAng_ = 0;
        
        // Last seen marker pose used for bridge crossing
        f32 relMarkerX_, relMarkerY_, relMarkerAng_;
        
        CarryState carryState_ = CARRY_NONE;
        bool lastActionSucceeded_ = false;
        
        // When to transition to the next state. Only some states use this.
        u32 transitionTime_ = 0;
        
        // Whether or not docking path should be traversed with manually controlled speed
        bool useManualSpeed_ = false;
        
      } // "private" namespace
      
      
      Mode GetMode() {
        return mode_;
      }

      Result Init() {
        Reset();
        return RESULT_OK;
      }

      
      void Reset()
      {
        mode_ = IDLE;
        relMarkerX_ = -1.f;
        DockingController::ResetDocker();
      }

      void UpdatePoseSnapshot()
      {
        Localization::GetCurrentMatPose(ptStamp_.x, ptStamp_.y, angleStamp_);
      }
      
      Result SendBlockPickUpMessage(const bool success)
      {
        BlockPickedUp msg;
        msg.timestamp = HAL::GetTimeStamp();
        msg.didSucceed = success;
        if(RobotInterface::SendMessage(msg)) {
          return RESULT_OK;
        }
        return RESULT_FAIL;
      }
      
      Result SendBlockPlacedMessage(const bool success)
      {
        BlockPlaced msg;
        msg.timestamp = HAL::GetTimeStamp();
        msg.didSucceed = success;
        if(RobotInterface::SendMessage(msg)) {
          return RESULT_OK;
        }
        return RESULT_FAIL;
      }
      
      static void StartBackingOut()
      {
        static const f32 MIN_BACKOUT_DIST = 10.f;
        
        DockingController::GetLastMarkerPose(relMarkerX_, relMarkerY_, relMarkerAng_);
        
        f32 backoutDist_mm = MIN_BACKOUT_DIST;
        if(relMarkerX_ > 0.f && relMarkerX_ <= BACKOUT_DISTANCE_MM) {
          backoutDist_mm = MAX(MIN_BACKOUT_DIST, BACKOUT_DISTANCE_MM - relMarkerX_);
        }
        
        const f32 backoutTime_sec = backoutDist_mm / BACKOUT_SPEED_MMPS;

        PRINT("PAP: Last marker dist = %.1fmm. Starting %.1fmm backout (%.2fsec duration)\n",
              relMarkerX_, backoutDist_mm, backoutTime_sec);
        
        transitionTime_ = HAL::GetTimeStamp() + (backoutTime_sec*1e3f);
        
        SteeringController::ExecuteDirectDrive(-BACKOUT_SPEED_MMPS, -BACKOUT_SPEED_MMPS);
        
        // Now that we've used relMarkerX_, mark it as used
        relMarkerX_ = -1.f;
        
        mode_ = BACKOUT;
      }
      
      Result Update()
      {
        Result retVal = RESULT_OK;
        
        switch(mode_)
        {
          case IDLE:
            ProxSensors::EnableCliffDetector(true);
            break;
            
          case SET_LIFT_PREDOCK:
          {
#if(DEBUG_PAP_CONTROLLER)
            PRINT("PAP: SETTING LIFT PREDOCK (action %d)\n", action_);
#endif
            mode_ = MOVING_LIFT_PREDOCK;
            LiftController::SetMaxSpeedAndAccel(DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
            switch(action_) {
              case DA_PICKUP_LOW:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK);
                dockOffsetDistX_ = ORIGIN_TO_LOW_LIFT_DIST_MM;
                break;
              case DA_PICKUP_HIGH:
                // This action starts by lowering the lift and tracking the high block
                LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK);
                dockOffsetDistX_ = ORIGIN_TO_HIGH_LIFT_DIST_MM;
                break;
              case DA_PLACE_LOW:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY);
                dockOffsetDistX_ += ORIGIN_TO_LOW_LIFT_DIST_MM;
                break;
              case DA_PLACE_LOW_BLIND:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY);
                break;
              case DA_PLACE_HIGH:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY);
                dockOffsetDistX_ += ORIGIN_TO_HIGH_PLACEMENT_DIST_MM;
                break;
              case DA_ROLL_LOW:
              case DA_POP_A_WHEELIE:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY);
                dockOffsetDistX_ = ORIGIN_TO_LOW_ROLL_DIST_MM;
                break;
              case DA_ALIGN:
                break;
              case DA_RAMP_ASCEND:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY);
                dockOffsetDistX_ = 0;
                break;
              case DA_RAMP_DESCEND:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY);
                dockOffsetDistX_ = 30; // can't wait until we are actually on top of the marker to say we're done!
                break;
              case DA_CROSS_BRIDGE:
                dockOffsetDistX_ = BRIDGE_ALIGNED_MARKER_DISTANCE;
                break;
              case DA_MOUNT_CHARGER:
                dockOffsetDistX_ = CHARGER_ALIGNED_MARKER_DISTANCE;
                break;
              default:
                PRINT("ERROR: Unknown PickAndPlaceAction %d\n", action_);
                mode_ = IDLE;
                break;
            }
            break;
          }

          case MOVING_LIFT_PREDOCK:
#if(DEBUG_PAP_CONTROLLER)
            PERIODIC_PRINT(200, "PAP: MLP %d %d\n", LiftController::IsInPosition(), HeadController::IsInPosition());
#endif
            if (LiftController::IsInPosition() && HeadController::IsInPosition()) {
              
              if (action_ == DA_PLACE_LOW_BLIND) {
                DockingController::StartDockingToRelPose(dockOffsetDistX_,
                                                         dockOffsetDistY_,
                                                         dockOffsetAng_,
                                                         useManualSpeed_);
              } else {
                // Set the distance to the marker beyond which
                // we should ignore docking error signals since the lift occludes our view anyway.
                u32 pointOfNoReturnDist = 0;
                switch(action_) {
                  case DA_PICKUP_HIGH:
                    pointOfNoReturnDist = HIGH_DOCK_POINT_OF_NO_RETURN_DIST_MM;
                    break;
                  case DA_PICKUP_LOW:
                  case DA_PLACE_HIGH:
                  case DA_ALIGN:
                    pointOfNoReturnDist = LOW_DOCK_POINT_OF_NO_RETURN_DIST_MM;
                    break;
                  default:
                    break;
                }

                DockingController::StartDocking(dockOffsetDistX_,
                                                dockOffsetDistY_,
                                                dockOffsetAng_,
                                                useManualSpeed_,
                                                pointOfNoReturnDist);
              }
              mode_ = DOCKING;
#if(DEBUG_PAP_CONTROLLER)
              PRINT("PAP: DOCKING\n");
#endif
              
              //if (action_ == DA_PICKUP_HIGH) {
              //  DockingController::TrackCamWithLift(true);
              //}
            }
            break;
            
          case DOCKING:
             
            if (!DockingController::IsBusy()) {

              //DockingController::TrackCamWithLift(false);
              
              if (DockingController::DidLastDockSucceed())
              {
                // Docking is complete
                DockingController::ResetDocker();
                
                // Take snapshot of pose
                UpdatePoseSnapshot();
                
                if (action_ == DA_ALIGN) {
                  #if(DEBUG_PAP_CONTROLLER)
                  PRINT("PAP: ALIGN\n");
                  #endif
                  mode_ = IDLE;
                } else if(action_ == DA_RAMP_DESCEND) {
                  #if(DEBUG_PAP_CONTROLLER)
                  PRINT("PAP: TRAVERSE_RAMP_DOWN\n");
                  #endif
                  // Start driving forward (blindly) -- wheel guides!
                  SteeringController::ExecuteDirectDrive(RAMP_TRAVERSE_SPEED_MMPS, RAMP_TRAVERSE_SPEED_MMPS);
                  mode_ = TRAVERSE_RAMP_DOWN;
                } else if (action_ == DA_CROSS_BRIDGE) {
                  #if(DEBUG_PAP_CONTROLLER)
                  PRINT("PAP: ENTER_BRIDGE\n");
                  #endif
                  // Start driving forward (blindly) -- wheel guides!
                  SteeringController::ExecuteDirectDrive(BRIDGE_TRAVERSE_SPEED_MMPS, BRIDGE_TRAVERSE_SPEED_MMPS);
                  mode_ = ENTER_BRIDGE;
                } else if (action_ == DA_MOUNT_CHARGER) {
                  #if(DEBUG_PAP_CONTROLLER)
                  PRINT("PAP: MOUNT_CHARGER\n");
                  #endif
                  f32 targetAngle = (Localization::GetCurrentMatOrientation() + PI_F).ToFloat();
                  SteeringController::ExecutePointTurn(targetAngle, 2, 10, 10, true);
                  mode_ = ROTATE_FOR_CHARGER_APPROACH;
                } else {
                  #if(DEBUG_PAP_CONTROLLER)
                  PRINT("PAP: SET_LIFT_POSTDOCK\n");
                  #endif
                  mode_ = SET_LIFT_POSTDOCK;
                }
                lastActionSucceeded_ = true;
              } else {
                // Block is not being tracked.
                // Probably not visible.
                #if(DEBUG_PAP_CONTROLLER)
                PRINT("WARN (PickAndPlaceController): Could not track block's marker\n");
                #endif
                // TODO: Send BTLE message notifying failure
                
                PRINT("PAP: Docking failed while picking/placing high or low. Backing out.\n");
                
                // Send failed pickup or place message
                switch(action_)
                {
                  case DA_PICKUP_LOW:
                  case DA_PICKUP_HIGH:
                  {
                    SendBlockPickUpMessage(false);
                    break;
                  } // PICKUP
                    
                  case DA_PLACE_LOW:
                  case DA_PLACE_LOW_BLIND:
                  case DA_PLACE_HIGH:
                  case DA_ROLL_LOW:
                  case DA_POP_A_WHEELIE:
                  {
                    SendBlockPlacedMessage(false);
                    break;
                  } // PLACE
                  default:
                    PRINT("ERROR: Reached default switch statement in DOCKING case.\n");
                } // switch(action_)

                
                // Switch to BACKOUT mode:
                lastActionSucceeded_ = false;
                StartBackingOut();
                //mode_ = IDLE;
              }
            }
            else if (action_ == DA_RAMP_ASCEND && (ABS(IMUFilter::GetPitch()) > ON_RAMP_ANGLE_THRESH) )
            {
              DockingController::ResetDocker();
              SteeringController::ExecuteDirectDrive(RAMP_TRAVERSE_SPEED_MMPS, RAMP_TRAVERSE_SPEED_MMPS);
              mode_ = TRAVERSE_RAMP;
              Localization::SetOnRamp(true);
            }
            break;

          case SET_LIFT_POSTDOCK:
          {
#if(DEBUG_PAP_CONTROLLER)
            PRINT("PAP: SETTING LIFT POSTDOCK\n");
#endif
            mode_ = MOVING_LIFT_POSTDOCK;
            switch(action_) {
              case DA_PLACE_LOW:
              case DA_PLACE_LOW_BLIND:
              {
                LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK);
                break;
              }
              case DA_PICKUP_LOW:
              case DA_PICKUP_HIGH:
              {
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY);
                break;
              }
              case DA_PLACE_HIGH:
              {
                LiftController::SetDesiredHeight(LIFT_HEIGHT_HIGHDOCK);
                break;
              }
              case DA_ROLL_LOW:
              {
                LiftController::SetMaxSpeedAndAccel(0.75, 100);
                LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK);
                mode_ = MOVING_LIFT_FOR_ROLL;
                break;
              }
              case DA_POP_A_WHEELIE:
                // Move lift down fast and drive forward fast
                LiftController::SetMaxSpeedAndAccel(10, 200);         // TODO: Restore these afterwards?
                LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK);
                SpeedController::SetUserCommandedAcceleration(100);   // TODO: Restore this accel afterwards?
                SteeringController::ExecuteDirectDrive(150, 150);
                transitionTime_ = HAL::GetTimeStamp() + 1000;
                mode_ = POPPING_A_WHEELIE;
                break;
              default:
                PRINT("ERROR: Unknown PickAndPlaceAction %d\n", action_);
                mode_ = IDLE;
                break;
            }
            break;
          }
          case MOVING_LIFT_FOR_ROLL:
          {
            if (LiftController::GetHeightMM() <= LIFT_HEIGHT_LOW_ROLL) {
              SteeringController::ExecuteDirectDrive(-60, -60);
              mode_ = MOVING_LIFT_POSTDOCK;
            }
            break;
          }
          case POPPING_A_WHEELIE:
            // Either the robot has pitched up, or timeout
            if (IMUFilter::GetPitch() > 1.2 || HAL::GetTimeStamp() > transitionTime_) {
              SteeringController::ExecuteDirectDrive(0, 0);
              mode_ = IDLE;
            }
            break;
          case MOVING_LIFT_POSTDOCK:
            if (LiftController::IsInPosition()) {
              LiftController::SetMaxSpeedAndAccel(DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
              
              // Backup after picking or placing,
              // and move the head to a position to admire our work
              
              // Set head angle
              switch(action_)
              {
                case DA_PICKUP_HIGH:
                case DA_PLACE_HIGH:
                {
                  HeadController::SetDesiredAngle(DEG_TO_RAD(15));
                  break;
                } // HIGH
                case DA_PICKUP_LOW:
                case DA_PLACE_LOW:
                case DA_PLACE_LOW_BLIND:
                case DA_ROLL_LOW:
                {
                  HeadController::SetDesiredAngle(DEG_TO_RAD(-15));
                  break;
                } // LOW
                default:
                  PRINT("ERROR: Reached default switch statement in MOVING_LIFT_POSTDOCK case.\n");
              } // switch(action_)
              
              // Send pickup or place message.  Assume success, let BaseStation
              // verify once we've backed out.
              switch(action_)
              {
                case DA_PICKUP_LOW:
                case DA_PICKUP_HIGH:
                {
                  SendBlockPickUpMessage(true);
                  carryState_ = CARRY_1_BLOCK;
                  break;
                } // PICKUP

                case DA_PLACE_LOW:
                case DA_PLACE_LOW_BLIND:
                case DA_PLACE_HIGH:
                case DA_ROLL_LOW:
                {
                  SendBlockPlacedMessage(true);
                  carryState_ = CARRY_NONE;
                  break;
                } // PLACE
                default:
                  PRINT("ERROR: Reached default switch statement in MOVING_LIFT_POSTDOCK case.\n");
              } // switch(action_)
              
              // Switch to BACKOUT
              StartBackingOut();
              

            } // if (LiftController::IsInPosition())
            break;
            
          case BACKOUT:
            if (HAL::GetTimeStamp() > transitionTime_)
            {
              SteeringController::ExecuteDirectDrive(0,0);
              
              if (HeadController::IsInPosition()) {
                mode_ = IDLE;
                lastActionSucceeded_ = true;
              }
            }
            break;
          case LOWER_LIFT:
            if (LiftController::IsInPosition()) {
#if(DEBUG_PAP_CONTROLLER)
              PRINT("PAP: IDLE\n");
#endif
              mode_ = IDLE;
              lastActionSucceeded_ = true;
              carryState_ = CARRY_NONE;
            }
            break;
            
          case TRAVERSE_RAMP_DOWN:
            if(IMUFilter::GetPitch() < -ON_RAMP_ANGLE_THRESH) {
#if(DEBUG_PAP_CONTROLLER)
              PRINT("PAP: Switching out of TRAVERSE_RAMP_DOWN to TRAVERSE_RAMP (angle = %f)\n", IMUFilter::GetPitch());
#endif
              Localization::SetOnRamp(true);
              mode_ = TRAVERSE_RAMP;
            }
            break;
            
          case TRAVERSE_RAMP:
            if ( ABS(IMUFilter::GetPitch()) < OFF_RAMP_ANGLE_THRESH ) {
              SteeringController::ExecuteDirectDrive(0, 0);
              #if(DEBUG_PAP_CONTROLLER)
              PRINT("PAP: IDLE (from TRAVERSE_RAMP)\n");
              #endif
              mode_ = IDLE;
              lastActionSucceeded_ = true;
              Localization::SetOnRamp(false);
            }
            break;
            
          case ENTER_BRIDGE:
            // Keep driving until the marker on the other side of the bridge is seen.
            if ( Localization::GetDistTo(ptStamp_.x, ptStamp_.y) > BRIDGE_ALIGNED_MARKER_DISTANCE) {
              // Set vision marker to look for marker
              //DockingController::StartTrackingOnly(dockToMarker2_, markerWidth_);
              UpdatePoseSnapshot();
              mode_ = TRAVERSE_BRIDGE;
              #if(DEBUG_PAP_CONTROLLER)
              PRINT("TRAVERSE_BRIDGE\n");
              #endif
              Localization::SetOnBridge(true);
            }
            break;
          case TRAVERSE_BRIDGE:
            if (DockingController::IsBusy()) {
              if (DockingController::GetLastMarkerPose(relMarkerX_, relMarkerY_, relMarkerAng_) && relMarkerX_ < 100.f) {
                // We're tracking the end marker.
                // Keep driving until we're off.
                UpdatePoseSnapshot();
                mode_ = LEAVE_BRIDGE;
                #if(DEBUG_PAP_CONTROLLER)
                PRINT("LEAVING_BRIDGE: relMarkerX = %f\n", relMarkerX_);
                #endif
              }
            } else {
              // Marker tracking timedout. Start it again.
              //DockingController::StartTrackingOnly(dockToMarker2_, markerWidth_);
              #if(DEBUG_PAP_CONTROLLER)
              PRINT("TRAVERSE_BRIDGE: Restarting tracking\n");
              #endif
            }
            break;
          case LEAVE_BRIDGE:
            if ( Localization::GetDistTo(ptStamp_.x, ptStamp_.y) > relMarkerX_ + MARKER_TO_OFF_BRIDGE_POSE_DIST) {
              SteeringController::ExecuteDirectDrive(0, 0);
              #if(DEBUG_PAP_CONTROLLER)
              PRINT("PAP: IDLE (from TRAVERSE_BRIDGE)\n");
              #endif
              mode_ = IDLE;
              lastActionSucceeded_ = true;
              Localization::SetOnBridge(false);
            }
            break;
          case ROTATE_FOR_CHARGER_APPROACH:
            if (SteeringController::GetMode() != SteeringController::SM_POINT_TURN) {
              // Move lift up, otherwise it drags on the ground when the robot gets on the ramp
              LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK + 15);
              
              // Start backing into charger
              SteeringController::ExecuteDirectDrive(-30, -30);
              transitionTime_ = HAL::GetTimeStamp() + 8000;
              
              mode_ = BACKUP_ON_CHARGER;
            }
            break;
          case BACKUP_ON_CHARGER:
            if (HAL::GetTimeStamp() > transitionTime_) {
              PRINT("BACKUP_ON_CHARGER timeout\n");
              SteeringController::ExecuteDirectDrive(0, 0);
              mode_ = IDLE;
              
              // TODO: Some kind of recovery?
              // ...
            } else if (HAL::BatteryIsOnCharger()) {
              SteeringController::ExecuteDirectDrive(0, 0);
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

      bool IsCarryingBlock()
      {
        return carryState_ != CARRY_NONE;
      }

      void SetCarryState(CarryState state)
      {
        carryState_ = state;
      }
      
      CarryState GetCarryState()
      {
        return carryState_;
      }
      
      void DockToBlock(const DockAction action,
                       const f32 rel_x,
                       const f32 rel_y,
                       const f32 rel_angle,
                       const bool useManualSpeed)
      {
#if(DEBUG_PAP_CONTROLLER)
        PRINT("PAP: DOCK TO BLOCK (action %d)\n", action);
#endif

        action_ = action;

        if (action_ == DA_PLACE_LOW_BLIND) {
          dockOffsetDistX_ = rel_x;
          dockOffsetDistY_ = rel_y;
          dockOffsetAng_ = rel_angle;
        } else {
          dockOffsetDistX_ = 0;
          dockOffsetDistY_ = 0;
          dockOffsetAng_ = 0;
        }
        
        useManualSpeed_ = useManualSpeed;
        
        relMarkerX_ = -1.f;
        
        mode_ = SET_LIFT_PREDOCK;
        lastActionSucceeded_ = false;
        
        if (action_ == DA_ROLL_LOW) {
          ProxSensors::EnableCliffDetector(false);
        }
      }

      void PlaceOnGround(const f32 rel_x, const f32 rel_y, const f32 rel_angle, const bool useManualSpeed)
      {
        DockToBlock(DA_PLACE_LOW_BLIND, rel_x, rel_y, rel_angle, useManualSpeed);
      }
      
    } // namespace PickAndPlaceController
  } // namespace Cozmo
} // namespace Anki
