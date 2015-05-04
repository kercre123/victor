#include "anki/common/robot/config.h"
#include "pickAndPlaceController.h"
#include "dockingController.h"
#include "headController.h"
#include "liftController.h"
#include "localization.h"
#include "imuFilter.h"

#include "anki/cozmo/shared/cozmoTypes.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
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
        const f32 BACKOUT_SPEED_MMPS = 40;
        
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

        Mode mode_ = IDLE;
        
        DockAction_t action_ = DA_PICKUP_LOW;

        Embedded::Point2f ptStamp_;
        Radians angleStamp_;
        
        f32 dockOffsetDistX_ = 0;
        f32 dockOffsetDistY_ = 0;
        f32 dockOffsetAng_ = 0;
        
        // Last seen marker pose used for bridge crossing
        f32 relMarkerX_, relMarkerY_, relMarkerAng_;
        
        CarryState_t carryState_ = CARRY_NONE;
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
        Messages::BlockPickedUp msg;
        msg.timestamp = HAL::GetTimeStamp();
        msg.didSucceed = success;
        if(HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::BlockPickedUp), &msg)) {
          return RESULT_OK;
        }
        return RESULT_FAIL;
      }
      
      Result SendBlockPlacedMessage(const bool success)
      {
        Messages::BlockPlaced msg;
        msg.timestamp = HAL::GetTimeStamp();
        msg.didSucceed = success;
        if(HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::BlockPlaced), &msg)) {
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
        
        transitionTime_ = HAL::GetMicroCounter() + (backoutTime_sec*1e6);
        
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
            break;
            
          case SET_LIFT_PREDOCK:
          {
#if(DEBUG_PAP_CONTROLLER)
            PRINT("PAP: SETTING LIFT PREDOCK (action %d)\n", action_);
#endif
            mode_ = MOVING_LIFT_PREDOCK;
            LiftController::SetMaxSpeedAndAccel(5, 10);
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
                break;
              case DA_PLACE_HIGH:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY);
                dockOffsetDistX_ = ORIGIN_TO_HIGH_PLACEMENT_DIST_MM;
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
              
              if (action_ == DA_PLACE_LOW) {
                DockingController::StartDockingToRelPose(dockOffsetDistX_,
                                                         dockOffsetDistY_,
                                                         dockOffsetAng_,
                                                         useManualSpeed_);
              } else {
                // When we are "docking" with a ramp or crossing a bridge, we
                // don't want to worry about the X angle being large (since we
                // _expect_ it to be large, since the markers are facing upward).
                const bool checkAngleX = !(action_ == DA_RAMP_ASCEND || action_ == DA_RAMP_DESCEND || action_ == DA_CROSS_BRIDGE);
                
                // Set the distance to the marker beyond which
                // we should ignore docking error signals since the lift occludes our view anyway.
                u32 pointOfNoReturnDist = 0;
                switch(action_) {
                  case DA_PICKUP_HIGH:
                    pointOfNoReturnDist = HIGH_DOCK_POINT_OF_NO_RETURN_DIST_MM;
                    break;
                  case DA_PICKUP_LOW:
                  case DA_PLACE_HIGH:
                    pointOfNoReturnDist = LOW_DOCK_POINT_OF_NO_RETURN_DIST_MM;
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
                
                if(action_ == DA_RAMP_DESCEND) {
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
                  case DA_PLACE_HIGH:
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
            switch(action_) {
              case DA_PLACE_LOW:
              {
                LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK);
                mode_ = MOVING_LIFT_POSTDOCK;
                break;
              }
              case DA_PICKUP_LOW:
              case DA_PICKUP_HIGH:
              {
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY);
                mode_ = MOVING_LIFT_POSTDOCK;
                break;
              }
                
              case DA_PLACE_HIGH:
              {
                LiftController::SetDesiredHeight(LIFT_HEIGHT_HIGHDOCK - LIFT_PLACE_HIGH_SLOP);
                mode_ = MOVING_LIFT_POSTDOCK;
                break;
              }
              default:
                PRINT("ERROR: Unknown PickAndPlaceAction %d\n", action_);
                mode_ = IDLE;
                break;
            }
            break;
          }
            
          case MOVING_LIFT_POSTDOCK:
            if (LiftController::IsInPosition()) {
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
                case DA_PLACE_HIGH:
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
            if (HAL::GetMicroCounter() > transitionTime_)
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

      void SetCarryState(CarryState_t state)
      {
        carryState_ = state;
      }
      
      CarryState_t GetCarryState()
      {
        return carryState_;
      }
      
      void DockToBlock(const bool useManualSpeed,
                       const DockAction_t action)
      {
#if(DEBUG_PAP_CONTROLLER)
        PRINT("PAP: DOCK TO BLOCK (action %d)\n", action);
#endif

        if (action == DA_PLACE_LOW) {
          PRINT("WARNING: Invalid action %d for DockToBlock(). Ignoring.\n", action);
          return;
        }
        
        action_ = action;
        
        dockOffsetDistX_ = 0;
        dockOffsetDistY_ = 0;
        dockOffsetAng_ = 0;
        
        useManualSpeed_ = useManualSpeed;
        
        relMarkerX_ = -1.f;
        
        mode_ = SET_LIFT_PREDOCK;
        lastActionSucceeded_ = false;
      }

      
      void PlaceOnGround(const f32 rel_x, const f32 rel_y, const f32 rel_angle, const bool useManualSpeed)
      {
        action_ = DA_PLACE_LOW;
        dockOffsetDistX_ = rel_x;
        dockOffsetDistY_ = rel_y;
        dockOffsetAng_ = rel_angle;
        
        useManualSpeed_ = useManualSpeed;
        
        relMarkerX_ = -1.f;
        
        mode_ = SET_LIFT_PREDOCK;
        lastActionSucceeded_ = false;
      }

      
    } // namespace PickAndPlaceController
  } // namespace Cozmo
} // namespace Anki
