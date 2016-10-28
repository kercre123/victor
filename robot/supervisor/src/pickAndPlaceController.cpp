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
#include "pathFollower.h"


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
        const f32 BACKOUT_DISTANCE_MM = 60;
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
        const u32 HIGH_DOCK_POINT_OF_NO_RETURN_DIST_MM = ORIGIN_TO_HIGH_LIFT_DIST_MM + 40;

        // Distance at which robot should start driving blind
        // along last generated docking path during PICKUP_LOW and PLACE_HIGH.
        const u32 LOW_DOCK_POINT_OF_NO_RETURN_DIST_MM = ORIGIN_TO_LOW_LIFT_DIST_MM + 10;
        
        const u32 MOUNT_CHARGER_POINT_OF_NO_RETURN_DIST_MM = 10;

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
        f32 dockDecel_mmps2_ = 0;

        // Distance to last known docking marker pose
        f32 lastMarkerDist_;

        CarryState carryState_ = CARRY_NONE;

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
        
        typedef enum
        {
          WAITING_TO_MOVE,
          MOVING_BACK,
          NOTHING,
        } PickupAnimMode;
        
        // State of the pickup animation
        PickupAnimMode pickupAnimMode_ = NOTHING;
        
        const u8 PICKUP_ANIM_SPEED_MMPS = 20;
        const u8 PICKUP_ANIM_DIST_MM = 8;
        const u8 PICKUP_ANIM_STARTING_DIST_MM = 20;
        const u8 PICKUP_ANIM_TRANSITION_TIME_MS = 100;
        const u8 PICKUP_ANIM_ACCEL_MMPS2 = 100;

        
        // Deep roll action params
        // Note: These are mainly for tuning the deep roll and can be removed once it's locked down.
        f32 _rollLiftHeight_mm = 35;        // The lift height to command when engaging the block for roll
        f32 _rollDriveSpeed_mmps = 50;      // The forward driving speed while engaging the block for roll
        f32 _rollDriveAccel_mmps2 = 40;     // The forward driving accel while engaging the block for roll
        u32 _rollDriveDuration_ms = 1500;   // The forward driving duration while engaging the block for roll
        f32 _rollBackupDist_mm = 100;       // The amount to back up once the lift has engaged the block for roll
        
        // Deep roll actions const params for a scooch adjustment
        // Seems to help avoid treads rubbing up against cube corners causing roll to fail
        const f32 _kRollLiftScoochSpeed_rad_per_sec = DEG_TO_RAD_F32(50);
        const f32 _kRollLiftScoochAccel_rad_per_sec_2 = DEG_TO_RAD_F32(50);
        const f32 _kRollLiftHeightScoochOffset_mm = 10;
        const f32 _kRollLiftScoochDuration_ms = 250;

        // Face plant parameters
        const f32 _facePlantLiftSpeed_radps = DEG_TO_RAD_F32(10000);
        const f32 _facePlantDriveSpeed_mmps = -500;
        const f32 _facePlantStartBackupPitch_rad = DEG_TO_RAD_F32(-25.f);
        const f32 _facePlantLiftTime_ms = 1000;
        const f32 _facePlantTimeout_ms = 500;
        
        const u32 _kPopAWheelieTimeout_ms = 500;
        
      } // "private" namespace


      Mode GetMode() {
        return mode_;
      }

      Result Init() {
        Reset();
        return RESULT_OK;
      }

      DockAction GetCurAction()
      {
        return action_;
      }

      void Reset()
      {
        mode_ = IDLE;
        DockingController::StopDocking();
        SteeringController::ExecuteDirectDrive(0, 0);
        ProxSensors::EnableCliffDetector(true);
        IMUFilter::EnablePickupDetect(true);
      }

      void UpdatePoseSnapshot()
      {
        Localization::GetCurrentMatPose(ptStamp_.x, ptStamp_.y, angleStamp_);
      }
      
      Result SendPickAndPlaceResultMessage(const bool success,
                                           const BlockStatus blockStatus)
      {
        PickAndPlaceResult msg;
        msg.timestamp = HAL::GetTimeStamp();
        msg.didSucceed = success;
        msg.blockStatus = blockStatus;
        msg.result = DockingController::GetDockingResult();
        
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
        static const f32 MIN_BACKOUT_DIST_MM = 35.f;

        f32 backoutDist_mm = 0;
        switch(action_)
        {
          case DA_PLACE_LOW_BLIND:
          case DA_ROLL_LOW:
          {
            backoutDist_mm = MIN_BACKOUT_DIST_MM;
            break;
          }
          case DA_DEEP_ROLL_LOW:
          {
            backoutDist_mm = _rollBackupDist_mm;
            break;
          }
          case DA_PICKUP_HIGH:
          case DA_PICKUP_LOW:
          case DA_PLACE_HIGH:
          case DA_PLACE_LOW:
          case DA_FACE_PLANT:
          {
            backoutDist_mm = BACKOUT_DISTANCE_MM;
            break;
          }
          default:
          {
            AnkiError( 285, "PAP.StartBackingOut.InvalidAction", 347, "%d", 1, action_);
          }
        }
        
        const f32 backoutTime_sec = backoutDist_mm / BACKOUT_SPEED_MMPS;

        AnkiInfo( 286, "PAP.StartBackingOut.Dist", 565, "Starting %.1fmm backout (%.2fsec duration)", 2, backoutDist_mm, backoutTime_sec);

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
            switch(action_) {
              case DA_PICKUP_LOW:
              case DA_FACE_PLANT:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK, DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
                dockOffsetDistX_ = ORIGIN_TO_LOW_LIFT_DIST_MM;
                break;
              case DA_PICKUP_HIGH:
                // This action starts by lowering the lift and tracking the high block
                LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK, DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
                dockOffsetDistX_ = ORIGIN_TO_HIGH_LIFT_DIST_MM;
                break;
              case DA_PLACE_LOW:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY, DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
                dockOffsetDistX_ += ORIGIN_TO_LOW_LIFT_DIST_MM;
                break;
              case DA_PLACE_LOW_BLIND:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY, DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
                break;
              case DA_PLACE_HIGH:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY, DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
                dockOffsetDistX_ += ORIGIN_TO_HIGH_PLACEMENT_DIST_MM;
                break;
              case DA_ROLL_LOW:
              case DA_DEEP_ROLL_LOW:
              case DA_POP_A_WHEELIE:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY, DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
                dockOffsetDistX_ = ORIGIN_TO_LOW_ROLL_DIST_MM;
                break;
              case DA_ALIGN:
                break;
              case DA_RAMP_ASCEND:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY, DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
                dockOffsetDistX_ = 0;
                break;
              case DA_RAMP_DESCEND:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY, DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
                dockOffsetDistX_ = 30; // can't wait until we are actually on top of the marker to say we're done!
                break;
              case DA_CROSS_BRIDGE:
                dockOffsetDistX_ = BRIDGE_ALIGNED_MARKER_DISTANCE;
                break;
              case DA_MOUNT_CHARGER:
                dockOffsetDistX_ = CHARGER_ALIGNED_MARKER_DISTANCE;
                break;
              default:
                AnkiError( 287, "PAP.SET_LIFT_PREDOCK.InvalidAction", 347, "%d", 1, action_);
                Reset();
                break;
            }
            break;
          }

          case MOVING_LIFT_PREDOCK:
            if (LiftController::IsInPosition() && HeadController::IsInPosition()) {

              if (action_ == DA_PLACE_LOW_BLIND) {
                DockingController::StartDockingToRelPose(dockSpeed_mmps_,
                                                         dockAccel_mmps2_,
                                                         dockDecel_mmps2_,
                                                         dockOffsetDistX_,
                                                         dockOffsetDistY_,
                                                         dockOffsetAng_,
                                                         useManualSpeed_);
              } else {

                // Set the distance to the marker beyond which
                // we should ignore docking error signals since the lift occludes our view anyway.
                bool useFirstErrorSignalOnly = false;
                u32 pointOfNoReturnDist = 0;
                switch(action_) {
                  case DA_PICKUP_HIGH:
                    pointOfNoReturnDist = HIGH_DOCK_POINT_OF_NO_RETURN_DIST_MM;
                    break;
                  case DA_PICKUP_LOW:
                  case DA_PLACE_HIGH:
                  case DA_ROLL_LOW:
                  case DA_DEEP_ROLL_LOW:
                  case DA_FACE_PLANT:
                  case DA_POP_A_WHEELIE:
                  case DA_ALIGN:
                    pointOfNoReturnDist = LOW_DOCK_POINT_OF_NO_RETURN_DIST_MM;
                    break;
                  case DA_MOUNT_CHARGER:
                    pointOfNoReturnDist = dockOffsetDistX_ + MOUNT_CHARGER_POINT_OF_NO_RETURN_DIST_MM;
                    useFirstErrorSignalOnly = true;
                    break;
                  default:
                    break;
                }

                DockingController::StartDocking(dockSpeed_mmps_,
                                                dockAccel_mmps2_,
                                                dockDecel_mmps2_,
                                                dockOffsetDistX_,
                                                dockOffsetDistY_,
                                                dockOffsetAng_,
                                                useManualSpeed_,
                                                pointOfNoReturnDist,
                                                useFirstErrorSignalOnly);
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
                DockingController::StopDocking();

                // Take snapshot of pose
                UpdatePoseSnapshot();

                if (action_ == DA_ALIGN) {
                  #if(DEBUG_PAP_CONTROLLER)
                  AnkiDebug( 14, "PAP", 123, "ALIGN", 0);
                  #endif
                  SendPickAndPlaceResultMessage(true, NO_BLOCK);
                  Reset();
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
                  Localization::GetDriveCenterPose(robotPose_x, robotPose_y, robotPose_angle);
                  const Anki::Embedded::Pose2d& markerPose = DockingController::GetLastMarkerAbsPose();
                  f32 relAngleToMarker = atan2_acc(markerPose.GetY() - robotPose_y, markerPose.GetX() - robotPose_x);
                  relAngleToMarker -= robotPose_angle.ToFloat();

                  f32 targetAngle = (Localization::GetCurrPose_angle() + PI_F + relAngleToMarker).ToFloat();
                  SteeringController::ExecutePointTurn(targetAngle, 2, 10, 10, DEG_TO_RAD_F32(1), true);
                  mode_ = ROTATE_FOR_CHARGER_APPROACH;
                } else {
                  #if(DEBUG_PAP_CONTROLLER)
                  AnkiDebug( 14, "PAP", 127, "SET_LIFT_POSTDOCK\n", 0);
                  #endif
                  mode_ = SET_LIFT_POSTDOCK;
                }
              } else {
                // Docking failed for some reason. Probably couldn't see block anymore.
                AnkiDebug( 363, "PAP.DOCKING.DockingFailed", 305, "", 0);

                // Send failed pickup or place message
                switch(action_)
                {
                  case DA_PICKUP_LOW:
                  case DA_PICKUP_HIGH:
                  {
                    SendPickAndPlaceResultMessage(false, BLOCK_PICKED_UP);
                    break;
                  } // PICKUP

                  case DA_PLACE_LOW:
                  case DA_PLACE_LOW_BLIND:
                  case DA_PLACE_HIGH:
                  case DA_ROLL_LOW:
                  case DA_DEEP_ROLL_LOW:
                  case DA_POP_A_WHEELIE:
                  {
                    SendPickAndPlaceResultMessage(false, BLOCK_PLACED);
                    break;
                  } // PLACE
                  case DA_ALIGN:
                  case DA_FACE_PLANT:
                  {
                    SendPickAndPlaceResultMessage(false, NO_BLOCK);
                    break;
                  }
                  default:
                    AnkiError( 289, "PAP.DOCKING.InvalidAction", 347, "%d", 1, action_);
                } // switch(action_)


                // Switch to BACKOUT mode:
                StartBackingOut();
              }
            }
            else if (action_ == DA_RAMP_ASCEND && (ABS(IMUFilter::GetPitch()) > ON_RAMP_ANGLE_THRESH) )
            {
              DockingController::StopDocking();
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
                LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK, DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
                transitionTime_ = HAL::GetTimeStamp() + START_BACKOUT_PLACE_LOW_TIMEOUT_MS;
                break;
              }
              case DA_PICKUP_LOW:
              case DA_PICKUP_HIGH:
              {
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY, DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
                
                // When a block is picked up we want an "animation" to play where Cozmo moves forwards and backwards
                // a little bit while picking up the block, gives a sense of momentum
                PathFollower::ClearPath();
                
                f32 x, y;
                Radians a;
                Localization::GetDriveCenterPose(x, y, a);
                
                PathFollower::AppendPathSegment_Line(0,
                                                     x-(PICKUP_ANIM_STARTING_DIST_MM)*cosf(a.ToFloat()),
                                                     y-(PICKUP_ANIM_STARTING_DIST_MM)*sinf(a.ToFloat()),
                                                     x+(PICKUP_ANIM_DIST_MM)*cosf(a.ToFloat()),
                                                     y+(PICKUP_ANIM_DIST_MM)*sinf(a.ToFloat()),
                                                     PICKUP_ANIM_SPEED_MMPS,
                                                     PICKUP_ANIM_ACCEL_MMPS2,
                                                     PICKUP_ANIM_ACCEL_MMPS2);
                
                PathFollower::StartPathTraversal();
                mode_ = PICKUP_ANIM;
                break;
              }
              case DA_PLACE_HIGH:
              {
                LiftController::SetDesiredHeight(LIFT_HEIGHT_HIGHDOCK, DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
                transitionTime_ = HAL::GetTimeStamp() + START_BACKOUT_PLACE_HIGH_TIMEOUT_MS;
                break;
              }
              case DA_ROLL_LOW:
              case DA_DEEP_ROLL_LOW:
              {
                ProxSensors::EnableCliffDetector(false);
                IMUFilter::EnablePickupDetect(false);

                // TODO: Can these be replaced with DEFAULT_LIFT_SPEED_RAD_PER_SEC and DEFAULT_LIFT_ACCEL_RAD_PER_SEC2?
                const f32 LIFT_SPEED_FOR_ROLL = 0.75f;
                const f32 LIFT_ACCEL_FOR_ROLL = 100.f;
                
                if (action_ == DA_DEEP_ROLL_LOW) {
                  LiftController::SetDesiredHeight(_rollLiftHeight_mm, LIFT_SPEED_FOR_ROLL, LIFT_ACCEL_FOR_ROLL);
                  SteeringController::ExecuteDirectDrive(_rollDriveSpeed_mmps, _rollDriveSpeed_mmps,
                                                         _rollDriveAccel_mmps2, _rollDriveAccel_mmps2);
                  transitionTime_ = HAL::GetTimeStamp() + _rollDriveDuration_ms;
                  mode_ = MOVING_LIFT_FOR_DEEP_ROLL;
                } else {
                  LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK, LIFT_SPEED_FOR_ROLL, LIFT_ACCEL_FOR_ROLL);
                  mode_ = MOVING_LIFT_FOR_ROLL;
                }
                break;
              }
              case DA_FACE_PLANT:
              {
                // Move lift up fast and drive backwards fast
                ProxSensors::EnableCliffDetector(false);
                IMUFilter::EnablePickupDetect(false);
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY, _facePlantLiftSpeed_radps);
                transitionTime_ = HAL::GetTimeStamp() + _facePlantLiftTime_ms;
                mode_ = FACE_PLANTING_BACKOUT;
                break;
              }
              case DA_POP_A_WHEELIE:
                // Move lift down fast and drive forward fast
                ProxSensors::EnableCliffDetector(false);
                LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK, 10, 200);
                SpeedController::SetUserCommandedAcceleration(100);   // TODO: Restore this accel afterwards?
                SteeringController::ExecuteDirectDrive(150, 150);
                transitionTime_ = HAL::GetTimeStamp() + _kPopAWheelieTimeout_ms;
                mode_ = POPPING_A_WHEELIE;
                break;
              default:
                AnkiError( 290, "PAP.SET_LIFT_POSTDOCK.InvalidAction", 347, "%d", 1, action_);
                Reset();
                break;
            }
            break;
          }
          case MOVING_LIFT_FOR_ROLL:
          {
            if (LiftController::GetHeightMM() <= LIFT_HEIGHT_LOW_ROLL) {
              SteeringController::ExecuteDirectDrive(-60, -60);
              
              // In case lift has trouble getting to low position, we don't want to back up forever.
              transitionTime_ = HAL::GetTimeStamp() + 1000;
              
              mode_ = MOVING_LIFT_POSTDOCK;
            }
            break;
          }
          case MOVING_LIFT_FOR_DEEP_ROLL:
          {
            if (HAL::GetTimeStamp() > transitionTime_ || IMUFilter::GetPitch() > DEG_TO_RAD_F32(35.f)) {
              
              // Just a little lift raise when the robot is most pitched.
              // Thinking this helps the lift scooch forward a bit more to make sure
              // it catches the lip of the corner.
              SteeringController::ExecuteDirectDrive(0, 0);
              LiftController::SetDesiredHeight(_rollLiftHeight_mm + _kRollLiftHeightScoochOffset_mm,
                                               _kRollLiftScoochSpeed_rad_per_sec, _kRollLiftScoochAccel_rad_per_sec_2);
              transitionTime_ = HAL::GetTimeStamp() + _kRollLiftScoochDuration_ms;
              mode_ = MOVING_LIFT_POSTDOCK;
            }
            break;
          }
          case FACE_PLANTING_BACKOUT:
          {
            if (IMUFilter::GetPitch() < _facePlantStartBackupPitch_rad) {
              SteeringController::ExecuteDirectDrive(_facePlantDriveSpeed_mmps, _facePlantDriveSpeed_mmps);
              transitionTime_ = HAL::GetTimeStamp() + _facePlantTimeout_ms;
              mode_ = FACE_PLANTING;
            } else if (HAL::GetTimeStamp() > transitionTime_) {
              SendPickAndPlaceResultMessage(false, NO_BLOCK);
              LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK, DEFAULT_LIFT_SPEED_RAD_PER_SEC);
              StartBackingOut();
            }
          }
          case FACE_PLANTING:
          {
            if (HAL::GetTimeStamp() > transitionTime_) {
              SendPickAndPlaceResultMessage(IMUFilter::GetPitch() < _facePlantStartBackupPitch_rad, NO_BLOCK);
              Reset();
            }
            break;
          }
          case POPPING_A_WHEELIE:
            // Either the robot has pitched up, or timeout
            if (IMUFilter::GetPitch() > 1.2f || HAL::GetTimeStamp() > transitionTime_) {
              SendPickAndPlaceResultMessage(true, NO_BLOCK);
              Reset();
            }
            break;
          case MOVING_LIFT_POSTDOCK:
            if (LiftController::IsInPosition() ||
                (transitionTime_ > 0 && transitionTime_ < HAL::GetTimeStamp())) {

              // Send pickup or place message.  Assume success, let BaseStation
              // verify once we've backed out.
              switch(action_)
              {
                case DA_PICKUP_LOW:
                case DA_PICKUP_HIGH:
                {
                  SendPickAndPlaceResultMessage(true, BLOCK_PICKED_UP);
                  carryState_ = CARRY_1_BLOCK;
                  break;
                } // PICKUP

                case DA_DEEP_ROLL_LOW:
                case DA_ROLL_LOW:
                {
                  LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK, DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
                
                  #ifdef SIMULATOR
                  // Prevents lift from attaching to block right after a roll
                  HAL::DisengageGripper();
                  #endif
                  
                  // Fall through...
                }
                case DA_PLACE_LOW:
                case DA_PLACE_LOW_BLIND:
                case DA_PLACE_HIGH:
                {
                  SendPickAndPlaceResultMessage(true, BLOCK_PLACED);
                  carryState_ = CARRY_NONE;
                  break;
                } // PLACE
                default:
                  AnkiError( 291, "PAP.MOVING_LIFT_POSTDOCK.InvalidAction", 347, "%d", 1, action_);
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
                Reset();
              }
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
              #if(DEBUG_PAP_CONTROLLER)
              AnkiDebug( 14, "PAP", 135, "IDLE (from TRAVERSE_RAMP)\n", 0);
              #endif
              Reset();
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
              #if(DEBUG_PAP_CONTROLLER)
              AnkiDebug( 14, "PAP", 139, "IDLE (from TRAVERSE_BRIDGE)\n", 0);
              #endif
              Reset();
              Localization::SetOnBridge(false);
            }
            break;
          case ROTATE_FOR_CHARGER_APPROACH:
            if (SteeringController::GetMode() != SteeringController::SM_POINT_TURN) {
              // Move lift up, otherwise it drags on the ground when the robot gets on the ramp
              LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK + 15, DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);

              // Start backing into charger
              SteeringController::ExecuteDirectDrive(-30, -30);
              transitionTime_ = HAL::GetTimeStamp() + 8000;

              mode_ = BACKUP_ON_CHARGER;
            }
            break;
          case BACKUP_ON_CHARGER:
            if (HAL::GetTimeStamp() > transitionTime_) {
              AnkiEvent( 292, "PAP.BACKUP_ON_CHARGER.Timeout", 305, "", 0);
              SendChargerMountCompleteMessage(false);
              Reset();
              // TODO: Some kind of recovery?
              // ...
            } else if (IMUFilter::GetPitch() < TILT_FAILURE_ANGLE_RAD) {
              // Check for tilt
              if (tiltedOnChargerStartTime_ == 0) {
                tiltedOnChargerStartTime_ = HAL::GetTimeStamp();
              } else if (HAL::GetTimeStamp() - tiltedOnChargerStartTime_ > TILT_FAILURE_DURATION_MS) {
                // Drive forward until no tilt or timeout
                AnkiEvent( 293, "PAP.BACKUP_ON_CHARGER.Tilted", 305, "", 0);
                SteeringController::ExecuteDirectDrive(40, 40);
                transitionTime_ = HAL::GetTimeStamp() + 2500;
                mode_ = DRIVE_FORWARD;
              }
            } else if (HAL::BatteryIsOnCharger()) {
              AnkiEvent( 294, "PAP.BACKUP_ON_CHARGER.Success", 305, "", 0);
              SendChargerMountCompleteMessage(true);
              Reset();
            } else {
              tiltedOnChargerStartTime_ = 0;
            }
            break;
          case DRIVE_FORWARD:
            // For failed charger mounting recovery only
            if (HAL::GetTimeStamp() > transitionTime_) {
              SendChargerMountCompleteMessage(false);
              Reset();
            }
            break;
          case PICKUP_ANIM:
          {
            switch(pickupAnimMode_)
            {
              case(WAITING_TO_MOVE):
              {
                // Once enough time passes start driving backwards for the backwards part of the "animation"
                if(HAL::GetTimeStamp() > transitionTime_)
                {
                  SteeringController::ExecuteDirectDrive(-PICKUP_ANIM_SPEED_MMPS, -PICKUP_ANIM_SPEED_MMPS);
                  transitionTime_ = HAL::GetTimeStamp() + PICKUP_ANIM_TRANSITION_TIME_MS;
                  pickupAnimMode_ = MOVING_BACK;
                }
                break;
              }
              case(MOVING_BACK):
              {
                // We have been driving backwards for long enough and the "animation" is complete
                if(HAL::GetTimeStamp() > transitionTime_)
                {
                  SteeringController::ExecuteDirectDrive(0, 0);
                  mode_ = MOVING_LIFT_POSTDOCK;
                  pickupAnimMode_ = NOTHING;
                }
                break;
              }
              case(NOTHING):
              {
                // If we have finished doing the forwards part of the "animation" transition to waiting to do the
                // backwards part
                if(!PathFollower::IsTraversingPath())
                {
                  transitionTime_ = HAL::GetTimeStamp() + PICKUP_ANIM_TRANSITION_TIME_MS;
                  pickupAnimMode_ = WAITING_TO_MOVE;
                }
                break;
              }
            }
            break;
          }
          default:
            Reset();
            AnkiError( 295, "PAP.Update.InvalidAction", 347, "%d", 1, action_);
            break;
        }

        return retVal;

      } // Update()

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
                       const f32 decel_mmps2,
                       const f32 rel_x,
                       const f32 rel_y,
                       const f32 rel_angle,
                       const bool useManualSpeed,
                       const u8 numRetries)
      {
#if(DEBUG_PAP_CONTROLLER)
        AnkiDebug( 296, "PAP.DockToBlock.Action", 347, "%d", 1, action);
#endif

        action_ = action;

        dockSpeed_mmps_ = speed_mmps;
        dockAccel_mmps2_ = accel_mmps2;
        dockDecel_mmps2_ = decel_mmps2;

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
        
        DockingController::SetMaxRetries(numRetries);

      }

      void PlaceOnGround(const f32 speed_mmps,
                         const f32 accel_mmps2,
                         const f32 decel_mmps2,
                         const f32 rel_x,
                         const f32 rel_y,
                         const f32 rel_angle,
                         const bool useManualSpeed)
      {
        DockToBlock(DA_PLACE_LOW_BLIND,
                    speed_mmps,
                    accel_mmps2,
                    decel_mmps2,
                    rel_x,
                    rel_y,
                    rel_angle,
                    useManualSpeed);
      }

      
      void SetRollActionParams(const f32 liftHeight_mm,
                               const f32 driveSpeed_mmps,
                               const f32 driveAccel_mmps2,
                               const u32 driveDuration_ms,
                               const f32 backupDist_mm)
      {
        AnkiDebug( 297, "PAP.SetRollActionParams", 567, "liftHeight: %f, speed: %f, accel: %f, duration %d, backupDist %f", 5,
                  liftHeight_mm, driveSpeed_mmps, driveAccel_mmps2, driveDuration_ms, backupDist_mm);
        
        _rollLiftHeight_mm = liftHeight_mm;
        _rollDriveSpeed_mmps = driveSpeed_mmps;
        _rollDriveAccel_mmps2 = driveAccel_mmps2;
        _rollDriveDuration_ms = driveDuration_ms;
        _rollBackupDist_mm = backupDist_mm;
        
      }
      
    } // namespace PickAndPlaceController
  } // namespace Cozmo
} // namespace Anki
