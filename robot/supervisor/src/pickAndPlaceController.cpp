#include "anki/common/robot/config.h"
#include "pickAndPlaceController.h"
#include <math.h>
#include "anki/common/constantsAndMacros.h"
#include "dockingController.h"
#include "headController.h"
#include "liftController.h"
#include "localization.h"
#include "imuFilter.h"
#include "proxSensors.h"
#include "trig_fast.h"
#include "anki/cozmo/robot/logging.h"
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

        // The distance from the last-observed position of the target that we'd
        // like to be after backing out
        const f32 BACKOUT_DISTANCE_MM = 50.f;
        const f32 BACKOUT_SPEED_MMPS = 60;
        
        // Max amount of time to wait for lift to get into position before backing out.
        // Used for placement only when the lift tends to get stuck due to block friction.
        const u32 START_BACKOUT_PLACE_HIGH_TIMEOUT_MS = 500;
        const u32 START_BACKOUT_PLACE_LOW_TIMEOUT_MS = 1000;

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

        f32 dockSpeed_mmps_ = 0;
        f32 dockAccel_mmps2_ = 0;

        // Distance to last known docking marker pose
        f32 lastMarkerDist_;

        CarryState carryState_ = CARRY_NONE;
        bool lastActionSucceeded_ = false;

        // When to transition to the next state. Only some states use this.
        u32 transitionTime_ = 0;

        // Time when robot first becomes tiled on charger (while docking)
        u32 tiltedOnChargerStartTime_ = 0;

        // Pitch angle at which Cozmo is probably having trouble backing up on charger
        const f32 TILT_FAILURE_ANGLE_RAD = DEG_TO_RAD_F32(-30);

        // Amount of time that Cozmo pitch needs to exceed TILT_FAILURE_ANGLE_RAD in order to fail at backing up on charger
        const u32 TILT_FAILURE_DURATION_MS = 250;

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

      Result SendMovingLiftPostDockMessage()
      {
        MovingLiftPostDock msg;
        msg.action = action_;
        if(RobotInterface::SendMessage(msg)) {
          return RESULT_OK;
        }
        return RESULT_FAIL;
      }
      
      Result SendChargerMountCompleteMessage(const bool success)
      {
        ChargerMountComplete msg;
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

        if (action_ == DA_PLACE_LOW_BLIND) {
          lastMarkerDist_ = 30.f;
        } else {
          lastMarkerDist_ = DockingController::GetDistToLastDockMarker();
        }

        f32 backoutDist_mm = MIN_BACKOUT_DIST;
        if(lastMarkerDist_ > 0.f && lastMarkerDist_ <= BACKOUT_DISTANCE_MM) {
          backoutDist_mm = MAX(MIN_BACKOUT_DIST, BACKOUT_DISTANCE_MM - lastMarkerDist_);
        }

        const f32 backoutTime_sec = backoutDist_mm / BACKOUT_SPEED_MMPS;

        AnkiInfo( 14, "PAP", 119, "Last marker dist = %.1fmm. Starting %.1fmm backout (%.2fsec duration)", 3,
              lastMarkerDist_, backoutDist_mm, backoutTime_sec);

        transitionTime_ = HAL::GetTimeStamp() + (backoutTime_sec*1e3f);

        SteeringController::ExecuteDirectDrive(-BACKOUT_SPEED_MMPS, -BACKOUT_SPEED_MMPS);

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
            AnkiDebug( 14, "PAP", 120, "SETTING LIFT PREDOCK (action %d)", 1, action_);
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
                AnkiError( 14, "PAP", 121, "Unknown PickAndPlaceAction %d", 1, action_);
                mode_ = IDLE;
                break;
            }
            break;
          }

          case MOVING_LIFT_PREDOCK:
            if (LiftController::IsInPosition() && HeadController::IsInPosition()) {

              if (action_ == DA_PLACE_LOW_BLIND) {
                DockingController::StartDockingToRelPose(dockSpeed_mmps_,
                                                         dockAccel_mmps2_,
                                                         dockOffsetDistX_,
                                                         dockOffsetDistY_,
                                                         dockOffsetAng_,
                                                         useManualSpeed_);
              } else {

#ifdef SIMULATOR
                // Prevents lift from attaching to block right after a roll
                if (action_ == DA_ROLL_LOW) {
                  HAL::DisengageGripper();
                }
#endif

                // Set the distance to the marker beyond which
                // we should ignore docking error signals since the lift occludes our view anyway.
                u32 pointOfNoReturnDist = 0;
                switch(action_) {
                  case DA_PICKUP_HIGH:
                    pointOfNoReturnDist = HIGH_DOCK_POINT_OF_NO_RETURN_DIST_MM;
                    break;
                  case DA_PICKUP_LOW:
                  case DA_PLACE_HIGH:
                  case DA_ROLL_LOW:
                  case DA_POP_A_WHEELIE:
                  case DA_ALIGN:
                    pointOfNoReturnDist = LOW_DOCK_POINT_OF_NO_RETURN_DIST_MM;
                    break;
                  default:
                    break;
                }

                DockingController::StartDocking(dockSpeed_mmps_,
                                                dockAccel_mmps2_,
                                                dockOffsetDistX_,
                                                dockOffsetDistY_,
                                                dockOffsetAng_,
                                                useManualSpeed_,
                                                pointOfNoReturnDist);
              }
              mode_ = DOCKING;
#if(DEBUG_PAP_CONTROLLER)
              AnkiDebug( 14, "PAP", 122, "DOCKING", 0);
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
                  AnkiDebug( 14, "PAP", 123, "ALIGN", 0);
                  #endif
                  mode_ = IDLE;
                } else if(action_ == DA_RAMP_DESCEND) {
                  #if(DEBUG_PAP_CONTROLLER)
                  AnkiDebug( 14, "PAP", 124, "TRAVERSE_RAMP_DOWN\n", 0);
                  #endif
                  // Start driving forward (blindly) -- wheel guides!
                  SteeringController::ExecuteDirectDrive(RAMP_TRAVERSE_SPEED_MMPS, RAMP_TRAVERSE_SPEED_MMPS);
                  mode_ = TRAVERSE_RAMP_DOWN;
                } else if (action_ == DA_CROSS_BRIDGE) {
                  #if(DEBUG_PAP_CONTROLLER)
                  AnkiDebug( 14, "PAP", 125, "ENTER_BRIDGE\n", 0);
                  #endif
                  // Start driving forward (blindly) -- wheel guides!
                  SteeringController::ExecuteDirectDrive(BRIDGE_TRAVERSE_SPEED_MMPS, BRIDGE_TRAVERSE_SPEED_MMPS);
                  mode_ = ENTER_BRIDGE;
                } else if (action_ == DA_MOUNT_CHARGER) {
                  #if(DEBUG_PAP_CONTROLLER)
                  AnkiDebug( 14, "PAP", 126, "MOUNT_CHARGER\n", 0);
                  #endif

                  // Compute angle to turn in order to face marker
                  f32 robotPose_x, robotPose_y;
                  Radians robotPose_angle;
                  Localization::GetCurrentMatPose(robotPose_x, robotPose_y, robotPose_angle);
                  const Anki::Embedded::Pose2d& markerPose = DockingController::GetLastMarkerAbsPose();
                  f32 relAngleToMarker = atan2_acc(markerPose.GetY() - robotPose_y, markerPose.GetX() - robotPose_x);
                  relAngleToMarker -= robotPose_angle.ToFloat();

                  f32 targetAngle = (Localization::GetCurrentMatOrientation() + PI_F + relAngleToMarker).ToFloat();
                  SteeringController::ExecutePointTurn(targetAngle, 2, 10, 10, true);
                  mode_ = ROTATE_FOR_CHARGER_APPROACH;
                } else {
                  #if(DEBUG_PAP_CONTROLLER)
                  AnkiDebug( 14, "PAP", 127, "SET_LIFT_POSTDOCK\n", 0);
                  #endif
                  mode_ = SET_LIFT_POSTDOCK;
                }
                lastActionSucceeded_ = true;
              } else {
                // Block is not being tracked.
                // Probably not visible.
                #if(DEBUG_PAP_CONTROLLER)
                AnkiWarn( 14, "PAP", 128, "Could not track block's marker", 0);
                #endif

                AnkiDebug( 14, "PAP", 129, "Docking failed while picking/placing high or low. Backing out.", 0);

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
                    AnkiError( 14, "PAP", 130, "Reached default switch statement in DOCKING case.", 0);
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
            AnkiDebug( 14, "PAP", 131, "SETTING LIFT POSTDOCK\n", 0);
#endif
            SendMovingLiftPostDockMessage();
            
            mode_ = MOVING_LIFT_POSTDOCK;
            switch(action_) {
              case DA_PLACE_LOW:
              case DA_PLACE_LOW_BLIND:
              {
                LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK);
                transitionTime_ = HAL::GetTimeStamp() + START_BACKOUT_PLACE_LOW_TIMEOUT_MS;
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
                transitionTime_ = HAL::GetTimeStamp() + START_BACKOUT_PLACE_HIGH_TIMEOUT_MS;
                break;
              }
              case DA_ROLL_LOW:
              {
                ProxSensors::EnableCliffDetector(false);
                LiftController::SetMaxSpeedAndAccel(0.75, 100);
                LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK);
                mode_ = MOVING_LIFT_FOR_ROLL;
                break;
              }
              case DA_POP_A_WHEELIE:
                // Move lift down fast and drive forward fast
                ProxSensors::EnableCliffDetector(false);
                LiftController::SetMaxSpeedAndAccel(10, 200);         // TODO: Restore these afterwards?
                LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK);
                SpeedController::SetUserCommandedAcceleration(100);   // TODO: Restore this accel afterwards?
                SteeringController::ExecuteDirectDrive(150, 150);
                transitionTime_ = HAL::GetTimeStamp() + 1000;
                mode_ = POPPING_A_WHEELIE;
                break;
              default:
                AnkiError( 14, "PAP", 121, "Unknown PickAndPlaceAction %d", 1, action_);
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
              SendBlockPlacedMessage(true);  // TODO: Send different message?
                                             //       Only doing this here so that the engine knows docking succeeded.
              mode_ = IDLE;
            }
            break;
          case MOVING_LIFT_POSTDOCK:
            if (LiftController::IsInPosition() ||
                (transitionTime_ > 0 && transitionTime_ < HAL::GetTimeStamp())) {
              LiftController::SetMaxSpeedAndAccel(DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);

              // Backup after picking or placing,
              // and move the head to a position to admire our work

              // Set head angle
              switch(action_)
              {
                case DA_PICKUP_HIGH:
                case DA_PLACE_HIGH:
                {
                  HeadController::SetDesiredAngle(DEG_TO_RAD(20));
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
                  AnkiError( 14, "PAP", 132, "Reached default switch statement in MOVING_LIFT_POSTDOCK case.", 0);
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
                  AnkiError( 14, "PAP", 132, "Reached default switch statement in MOVING_LIFT_POSTDOCK case.", 0);
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
                ProxSensors::EnableCliffDetector(true);
                mode_ = IDLE;
                lastActionSucceeded_ = true;
              }
            }
            break;
          case LOWER_LIFT:
            if (LiftController::IsInPosition()) {
#if(DEBUG_PAP_CONTROLLER)
              AnkiDebug( 14, "PAP", 133, "IDLE\n", 0);
#endif
              mode_ = IDLE;
              lastActionSucceeded_ = true;
              carryState_ = CARRY_NONE;
            }
            break;

          case TRAVERSE_RAMP_DOWN:
            if(IMUFilter::GetPitch() < -ON_RAMP_ANGLE_THRESH) {
#if(DEBUG_PAP_CONTROLLER)
              AnkiDebug( 14, "PAP", 134, "Switching out of TRAVERSE_RAMP_DOWN to TRAVERSE_RAMP (angle = %f)\n", 1, IMUFilter::GetPitch());
#endif
              Localization::SetOnRamp(true);
              mode_ = TRAVERSE_RAMP;
            }
            break;

          case TRAVERSE_RAMP:
            if ( ABS(IMUFilter::GetPitch()) < OFF_RAMP_ANGLE_THRESH ) {
              SteeringController::ExecuteDirectDrive(0, 0);
              #if(DEBUG_PAP_CONTROLLER)
              AnkiDebug( 14, "PAP", 135, "IDLE (from TRAVERSE_RAMP)\n", 0);
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
              AnkiDebug( 14, "PAP", 136, "TRAVERSE_BRIDGE", 0);
              #endif
              Localization::SetOnBridge(true);
            }
            break;
          case TRAVERSE_BRIDGE:
            if (DockingController::IsBusy()) {
              lastMarkerDist_ = DockingController::GetDistToLastDockMarker();
              if (lastMarkerDist_ < 100.f) {
                // We're tracking the end marker.
                // Keep driving until we're off.
                UpdatePoseSnapshot();
                mode_ = LEAVE_BRIDGE;
                #if(DEBUG_PAP_CONTROLLER)
                AnkiDebug( 14, "PAP", 137, "LEAVING_BRIDGE: relMarkerX = %f", 1, lastMarkerDist_);
                #endif
              }
            } else {
              // Marker tracking timedout. Start it again.
              //DockingController::StartTrackingOnly(dockToMarker2_, markerWidth_);
              #if(DEBUG_PAP_CONTROLLER)
              AnkiDebug( 14, "PAP", 138, "TRAVERSE_BRIDGE: Restarting tracking", 0);
              #endif
            }
            break;
          case LEAVE_BRIDGE:
            if ( Localization::GetDistTo(ptStamp_.x, ptStamp_.y) > lastMarkerDist_ + MARKER_TO_OFF_BRIDGE_POSE_DIST) {
              SteeringController::ExecuteDirectDrive(0, 0);
              #if(DEBUG_PAP_CONTROLLER)
              AnkiDebug( 14, "PAP", 139, "IDLE (from TRAVERSE_BRIDGE)\n", 0);
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
              AnkiEvent( 15, "BACKUP_ON_CHARGER", 140, "timeout", 0);
              SteeringController::ExecuteDirectDrive(0, 0);
              SendChargerMountCompleteMessage(false);
              mode_ = IDLE;

              // TODO: Some kind of recovery?
              // ...
            } else if (IMUFilter::GetPitch() < TILT_FAILURE_ANGLE_RAD) {
              // Check for tilt
              if (tiltedOnChargerStartTime_ == 0) {
                tiltedOnChargerStartTime_ = HAL::GetTimeStamp();
              } else if (HAL::GetTimeStamp() - tiltedOnChargerStartTime_ > TILT_FAILURE_DURATION_MS) {
                // Drive forward until no tilt or timeout
                AnkiEvent( 15, "BACKUP_ON_CHARGER", 141, "tilted", 0);
                SteeringController::ExecuteDirectDrive(40, 40);
                transitionTime_ = HAL::GetTimeStamp() + 2500;
                mode_ = DRIVE_FORWARD;
              }
            } else if (HAL::BatteryIsOnCharger()) {
              AnkiEvent( 15, "BACKUP_ON_CHARGER", 142, "success", 0);
              SteeringController::ExecuteDirectDrive(0, 0);
              SendChargerMountCompleteMessage(true);
              lastActionSucceeded_ = true;
              mode_ = IDLE;
            } else {
              tiltedOnChargerStartTime_ = 0;
            }
            break;
          case DRIVE_FORWARD:
            // For failed charger mounting recovery only
            if (HAL::GetTimeStamp() > transitionTime_) {
              SteeringController::ExecuteDirectDrive(0, 0);
              SendChargerMountCompleteMessage(false);
              mode_ = IDLE;
            }
            break;
          default:
            mode_ = IDLE;
            AnkiError( 14, "PAP", 39, "Reached default case in DockingController "
                  "mode switch statement.(1)", 0);
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
                       const f32 speed_mmps,
                       const f32 accel_mmps2,
                       const f32 rel_x,
                       const f32 rel_y,
                       const f32 rel_angle,
                       const bool useManualSpeed)
      {
#if(DEBUG_PAP_CONTROLLER)
        AnkiDebug( 14, "PAP", 143, "DOCK TO BLOCK (action %d)", 1, action);
#endif

        action_ = action;

        dockSpeed_mmps_ = speed_mmps;
        dockAccel_mmps2_ = accel_mmps2;

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

        transitionTime_ = 0;
        mode_ = SET_LIFT_PREDOCK;
        lastActionSucceeded_ = false;

      }

      void PlaceOnGround(const f32 speed_mmps,
                         const f32 accel_mmps2,
                         const f32 rel_x,
                         const f32 rel_y,
                         const f32 rel_angle,
                         const bool useManualSpeed)
      {
        DockToBlock(DA_PLACE_LOW_BLIND,
                    speed_mmps,
                    accel_mmps2,
                    rel_x,
                    rel_y,
                    rel_angle,
                    useManualSpeed);
      }

    } // namespace PickAndPlaceController
  } // namespace Cozmo
} // namespace Anki
