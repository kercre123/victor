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

        // Distance at which robot should start driving blind
        // along last generated docking path during DockAction::DA_PICKUP_HIGH.
        const u32 HIGH_DOCK_POINT_OF_NO_RETURN_DIST_MM = ORIGIN_TO_HIGH_LIFT_DIST_MM + 40;

        // Distance at which robot should start driving blind
        // along last generated docking path during PICKUP_LOW and PLACE_HIGH.
        const u32 LOW_DOCK_POINT_OF_NO_RETURN_DIST_MM = ORIGIN_TO_LOW_LIFT_DIST_MM + 10;

        const f32 DEFAULT_LIFT_SPEED_RAD_PER_SEC = 1.5;
        const f32 DEFAULT_LIFT_ACCEL_RAD_PER_SEC2 = 10;

        Mode mode_ = IDLE;

        DockAction action_ = DockAction::DA_PICKUP_LOW;

        // Whether or not to check for load on lift after docking.
        // Only relevant for pickup actions.
        bool doLiftLoadCheck_;

        Embedded::Point2f ptStamp_;
        Radians angleStamp_;

        f32 dockOffsetDistX_ = 0;
        f32 dockOffsetDistY_ = 0;
        f32 dockOffsetAng_ = 0;

        f32 dockSpeed_mmps_ = 0;
        f32 dockAccel_mmps2_ = 0;
        f32 dockDecel_mmps2_ = 0;

        CarryState carryState_ = CarryState::CARRY_NONE;

        // When to transition to the next state. Only some states use this.
        u32 transitionTime_ = 0;

        // Time when robot first becomes tilted on charger (while docking)
        u32 tiltedOnChargerStartTime_ = 0;

        // For charger mounting, whether or not to use cliff sensors to align the robot.
        bool useCliffSensorAlignment_ = false;

        // TODO: Default this to false once we have enough dark gray chargers (VIC-2755)
#ifdef  SIMULATOR
        bool chargerStripeIsBlack_ = false; // simulated chargers are "correct": dark gray with white stripe
#else
        bool chargerStripeIsBlack_ = true;
#endif

        // Threshold used to distinguish black stripe on charger from the white body
#ifdef SIMULATOR
        const u16 kChargerCliffBlackThreshold = 880;
#else
        const u16 kChargerCliffBlackThreshold = 400;
#endif

        // Charger docking wheel speeds for backing onto the charger:
        const float kChargerDockingSpeedHigh = -30.f;
        const float kChargerDockingSpeedLow  = -10.f;

        // Pitch angle at which Cozmo is probably having trouble backing up on charger
        const f32 TILT_FAILURE_ANGLE_RAD = DEG_TO_RAD_F32(-30);

        // After excessive pitch angle is detected and the robot begins driving forward,
        // this is the angle above which the robot is considered to have recovered
        const f32 TILT_FAILURE_RECOVERY_ANGLE_RAD = DEG_TO_RAD_F32(-10);

        // Amount of time that Cozmo pitch needs to exceed TILT_FAILURE_ANGLE_RAD in order to fail at backing up on charger
        const u32 TILT_FAILURE_DURATION_MS = 250;

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

      void SetChargerStripeIsBlack(const bool b) {
        chargerStripeIsBlack_ = b;
      }

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
        Localization::GetCurrPose(ptStamp_.x, ptStamp_.y, angleStamp_);
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
          case DockAction::DA_PLACE_LOW_BLIND:
          case DockAction::DA_ROLL_LOW:
          case DockAction::DA_POST_DOCK_ROLL:
          {
            backoutDist_mm = MIN_BACKOUT_DIST_MM;
            break;
          }
          case DockAction::DA_DEEP_ROLL_LOW:
          {
            backoutDist_mm = _rollBackupDist_mm;
            break;
          }
          case DockAction::DA_PICKUP_HIGH:
          case DockAction::DA_PICKUP_LOW:
            if (doLiftLoadCheck_) {
              // Only do CheckForLoad() for pickup actions. Fall through.
              LiftController::CheckForLoad();
            }
          case DockAction::DA_PLACE_HIGH:
          case DockAction::DA_PLACE_LOW:
          case DockAction::DA_FACE_PLANT:
          {
            backoutDist_mm = BACKOUT_DISTANCE_MM;
            break;
          }
          default:
          {
            AnkiError( "PAP.StartBackingOut.InvalidAction", "%hhu", action_);
          }
        }

        const f32 backoutTime_sec = backoutDist_mm / BACKOUT_SPEED_MMPS;

        AnkiInfo( "PAP.StartBackingOut.Dist", "Starting %.1fmm backout (%.2fsec duration)", backoutDist_mm, backoutTime_sec);

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
            AnkiDebug( "PAP", "SETTING LIFT PREDOCK (action %hhu)", action_);
#endif
            mode_ = MOVING_LIFT_PREDOCK;
            switch(action_) {
              case DockAction::DA_PICKUP_LOW:
              case DockAction::DA_FACE_PLANT:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK, DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
                dockOffsetDistX_ = ORIGIN_TO_LOW_LIFT_DIST_MM;
                break;
              case DockAction::DA_PICKUP_HIGH:
                // This action starts by lowering the lift and tracking the high block
                LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK, DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
                dockOffsetDistX_ = ORIGIN_TO_HIGH_LIFT_DIST_MM;
                break;
              case DockAction::DA_PLACE_LOW:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY, DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
                dockOffsetDistX_ += ORIGIN_TO_LOW_LIFT_DIST_MM;
                break;
              case DockAction::DA_PLACE_LOW_BLIND:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY, DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
                break;
              case DockAction::DA_PLACE_HIGH:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY, DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
                dockOffsetDistX_ += ORIGIN_TO_HIGH_PLACEMENT_DIST_MM;
                break;
              case DockAction::DA_ROLL_LOW:
              case DockAction::DA_DEEP_ROLL_LOW:
              case DockAction::DA_POP_A_WHEELIE:
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY, DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
                dockOffsetDistX_ = ORIGIN_TO_LOW_ROLL_DIST_MM;
                break;
              case DockAction::DA_ALIGN:
              case DockAction::DA_ALIGN_SPECIAL:
                dockOffsetDistX_ = ORIGIN_TO_LOW_LIFT_DIST_MM;
                break;
              case DockAction::DA_POST_DOCK_ROLL:
                // Skip docking completely and go straight to Setting lift for Post Dock
                mode_ = SET_LIFT_POSTDOCK;
                break;
              case DockAction::DA_BACKUP_ONTO_CHARGER:
              case DockAction::DA_BACKUP_ONTO_CHARGER_USE_CLIFF:
                ProxSensors::EnableCliffDetector(false);
                SteeringController::ExecuteDirectDrive(kChargerDockingSpeedHigh, kChargerDockingSpeedHigh);
                transitionTime_ = HAL::GetTimeStamp() + 7000;
                useCliffSensorAlignment_ = (action_ == DockAction::DA_BACKUP_ONTO_CHARGER_USE_CLIFF);
                mode_ = BACKUP_ON_CHARGER;
                break;
              default:
                AnkiError( "PAP.SET_LIFT_PREDOCK.InvalidAction", "%hhu", action_);
                Reset();
                break;
            }
            break;
          }

          case MOVING_LIFT_PREDOCK:
          {
            if (LiftController::IsInPosition() && HeadController::IsInPosition()) {

              if (action_ == DockAction::DA_PLACE_LOW_BLIND) {
                DockingController::StartDockingToRelPose(dockSpeed_mmps_,
                                                         dockAccel_mmps2_,
                                                         dockDecel_mmps2_,
                                                         dockOffsetDistX_,
                                                         dockOffsetDistY_,
                                                         dockOffsetAng_);
              } else {

                // Set the distance to the marker beyond which
                // we should ignore docking error signals since the lift occludes our view anyway.
                bool useFirstErrorSignalOnly = false;
                u32 pointOfNoReturnDist = 0;
                switch(action_) {
                  case DockAction::DA_PICKUP_HIGH:
                    pointOfNoReturnDist = HIGH_DOCK_POINT_OF_NO_RETURN_DIST_MM;
                    break;
                  case DockAction::DA_PICKUP_LOW:
                  case DockAction::DA_PLACE_HIGH:
                  case DockAction::DA_ROLL_LOW:
                  case DockAction::DA_DEEP_ROLL_LOW:
                  case DockAction::DA_FACE_PLANT:
                  case DockAction::DA_POP_A_WHEELIE:
                  case DockAction::DA_ALIGN:
                  case DockAction::DA_ALIGN_SPECIAL:
                    pointOfNoReturnDist = LOW_DOCK_POINT_OF_NO_RETURN_DIST_MM;
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
                                                pointOfNoReturnDist,
                                                useFirstErrorSignalOnly);
              }
              mode_ = DOCKING;
#if(DEBUG_PAP_CONTROLLER)
              AnkiDebug( "PAP", "DOCKING");
#endif
            }
            break;
          }
          case DOCKING:
          {
            if (!DockingController::IsBusy())
            {
              if (DockingController::DidLastDockSucceed())
              {
                // Docking is complete
                DockingController::StopDocking();

                // Take snapshot of pose
                UpdatePoseSnapshot();

                if (action_ == DockAction::DA_ALIGN ||
                    action_ == DockAction::DA_ALIGN_SPECIAL) {
                  #if(DEBUG_PAP_CONTROLLER)
                  AnkiDebug( "PAP", "ALIGN");
                  #endif
                  SendPickAndPlaceResultMessage(true, BlockStatus::NO_BLOCK);
                  Reset();
                } else {
                  #if(DEBUG_PAP_CONTROLLER)
                  AnkiDebug( "PAP", "SET_LIFT_POSTDOCK\n");
                  #endif
                  mode_ = SET_LIFT_POSTDOCK;
                }
              } else {
                // Docking failed for some reason. Probably couldn't see block anymore.
                AnkiDebug( "PAP.DOCKING.DockingFailed", "");

                // Send failed pickup or place message
                bool doBackup = true;
                switch(action_)
                {
                  case DockAction::DA_PICKUP_LOW:
                  case DockAction::DA_PICKUP_HIGH:
                  {
                    SendPickAndPlaceResultMessage(false, BlockStatus::BLOCK_PICKED_UP);
                    break;
                  } // PICKUP

                  case DockAction::DA_PLACE_LOW:
                  case DockAction::DA_PLACE_LOW_BLIND:
                  case DockAction::DA_PLACE_HIGH:
                  case DockAction::DA_ROLL_LOW:
                  case DockAction::DA_DEEP_ROLL_LOW:
                  case DockAction::DA_POP_A_WHEELIE:
                  {
                    SendPickAndPlaceResultMessage(false, BlockStatus::BLOCK_PLACED);
                    break;
                  } // PLACE
                  case DockAction::DA_ALIGN:
                  case DockAction::DA_ALIGN_SPECIAL:
                  case DockAction::DA_FACE_PLANT:
                  {
                    SendPickAndPlaceResultMessage(false, BlockStatus::NO_BLOCK);
                    Reset();
                    doBackup = false;
                    break;
                  }
                  default:
                    AnkiError( "PAP.DOCKING.InvalidAction", "%hhu", action_);
                } // switch(action_)


                // Switch to BACKOUT mode:
                if (doBackup) {
                  StartBackingOut();
                }
              }
            }
            break;
          }
          case SET_LIFT_POSTDOCK:
          {
#if(DEBUG_PAP_CONTROLLER)
            AnkiDebug( "PAP", "SETTING LIFT POSTDOCK\n");
#endif
            SendMovingLiftPostDockMessage();

            mode_ = MOVING_LIFT_POSTDOCK;
            switch(action_) {
              case DockAction::DA_PLACE_LOW:
              case DockAction::DA_PLACE_LOW_BLIND:
              {
                LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK, DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
                transitionTime_ = HAL::GetTimeStamp() + START_BACKOUT_PLACE_LOW_TIMEOUT_MS;
                break;
              }
              case DockAction::DA_PICKUP_LOW:
              case DockAction::DA_PICKUP_HIGH:
              {
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY, DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);

                // When a block is picked up we want an "animation" to play where Cozmo moves forwards and backwards
                // a little bit while picking up the block, gives a sense of momentum
                PathFollower::ClearPath();

                f32 x, y;
                Radians a;
                Localization::GetDriveCenterPose(x, y, a);

                PathFollower::AppendPathSegment_Line(x-(PICKUP_ANIM_STARTING_DIST_MM)*cosf(a.ToFloat()),
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
              case DockAction::DA_PLACE_HIGH:
              {
                LiftController::SetDesiredHeight(LIFT_HEIGHT_HIGHDOCK, DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);
                transitionTime_ = HAL::GetTimeStamp() + START_BACKOUT_PLACE_HIGH_TIMEOUT_MS;
                break;
              }
              case DockAction::DA_ROLL_LOW:
              case DockAction::DA_DEEP_ROLL_LOW:
              case DockAction::DA_POST_DOCK_ROLL:
              {
                ProxSensors::EnableCliffDetector(false);
                IMUFilter::EnablePickupDetect(false);

                // TODO: Can these be replaced with DEFAULT_LIFT_SPEED_RAD_PER_SEC and DEFAULT_LIFT_ACCEL_RAD_PER_SEC2?
                const f32 LIFT_SPEED_FOR_ROLL = 0.75f;
                const f32 LIFT_ACCEL_FOR_ROLL = 100.f;

                if (action_ == DockAction::DA_DEEP_ROLL_LOW) {
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
              case DockAction::DA_FACE_PLANT:
              {
                // Move lift up fast and drive backwards fast
                ProxSensors::EnableCliffDetector(false);
                IMUFilter::EnablePickupDetect(false);
                LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY, _facePlantLiftSpeed_radps);
                transitionTime_ = HAL::GetTimeStamp() + _facePlantLiftTime_ms;
                mode_ = FACE_PLANTING_BACKOUT;
                break;
              }
              case DockAction::DA_POP_A_WHEELIE:
                // Move lift down fast and drive forward fast
                ProxSensors::EnableCliffDetector(false);
                LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK, 10, 200);
                SpeedController::SetUserCommandedAcceleration(100);   // TODO: Restore this accel afterwards?
                SteeringController::ExecuteDirectDrive(150, 150);
                transitionTime_ = HAL::GetTimeStamp() + _kPopAWheelieTimeout_ms;
                mode_ = POPPING_A_WHEELIE;
                break;
              default:
                AnkiError( "PAP.SET_LIFT_POSTDOCK.InvalidAction", "%hhu", action_);
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
              SendPickAndPlaceResultMessage(false, BlockStatus::NO_BLOCK);
              LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK, DEFAULT_LIFT_SPEED_RAD_PER_SEC);
              StartBackingOut();
            }
          }
          case FACE_PLANTING:
          {
            if (HAL::GetTimeStamp() > transitionTime_) {
              SendPickAndPlaceResultMessage(IMUFilter::GetPitch() < _facePlantStartBackupPitch_rad, BlockStatus::NO_BLOCK);
              Reset();
            }
            break;
          }
          case POPPING_A_WHEELIE:
          {
            // Either the robot has pitched up, or timeout
            if (IMUFilter::GetPitch() > 1.2f || HAL::GetTimeStamp() > transitionTime_) {
              SendPickAndPlaceResultMessage(true, BlockStatus::NO_BLOCK);
              Reset();
            }
            break;
          }
          case MOVING_LIFT_POSTDOCK:
          {
            if (LiftController::IsInPosition() ||
                (transitionTime_ > 0 && transitionTime_ < HAL::GetTimeStamp())) {

              // Send pickup or place message.  Assume success, let BaseStation
              // verify once we've backed out.
              switch(action_)
              {
                case DockAction::DA_PICKUP_LOW:
                case DockAction::DA_PICKUP_HIGH:
                {
                  SendPickAndPlaceResultMessage(true, BlockStatus::BLOCK_PICKED_UP);
                  carryState_ = CarryState::CARRY_1_BLOCK;
                  break;
                } // PICKUP

                case DockAction::DA_DEEP_ROLL_LOW:
                case DockAction::DA_ROLL_LOW:
                case DockAction::DA_POST_DOCK_ROLL:
                {
                  LiftController::SetDesiredHeight(LIFT_HEIGHT_LOWDOCK, DEFAULT_LIFT_SPEED_RAD_PER_SEC, DEFAULT_LIFT_ACCEL_RAD_PER_SEC2);

                  #ifdef SIMULATOR
                  // Prevents lift from attaching to block right after a roll
                  HAL::DisengageGripper();
                  #endif

                  // Fall through...
                }
                case DockAction::DA_PLACE_LOW:
                case DockAction::DA_PLACE_LOW_BLIND:
                case DockAction::DA_PLACE_HIGH:
                {
                  SendPickAndPlaceResultMessage(true, BlockStatus::BLOCK_PLACED);
                  carryState_ = CarryState::CARRY_NONE;
                  break;
                } // PLACE
                default:
                  AnkiError( "PAP.MOVING_LIFT_POSTDOCK.InvalidAction", "%hhu", action_);
              } // switch(action_)

              // Switch to BACKOUT
              StartBackingOut();

            } // if (LiftController::IsInPosition())
            break;
          }
          case BACKOUT:
          {
            if (HAL::GetTimeStamp() > transitionTime_)
            {
              SteeringController::ExecuteDirectDrive(0,0);

              if (HeadController::IsInPosition()) {
                Reset();
              }
            }
            break;
          }
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
          case BACKUP_ON_CHARGER:
          {
            if (HAL::GetTimeStamp() > transitionTime_) {
              AnkiInfo("PAP.BACKUP_ON_CHARGER.Timeout", "");
              SendChargerMountCompleteMessage(false);
              Reset();
            } else if (ProxSensors::GetCliffDetectedFlags() == ((1<<HAL::CLIFF_BL) | (1<<HAL::CLIFF_BR))) {
              // Cliff detection is disabled now, so double check that we are not seeing a cliff
              // on BOTH rear cliff sensors (which may indicate a real cliff and not a spurious trip)
              AnkiInfo("PAP.BACKUP_ON_CHARGER.Cliff", "");
              SendChargerMountCompleteMessage(false);
              Reset();
            } else if (IMUFilter::GetPitch() < TILT_FAILURE_ANGLE_RAD) {
              // Check for excessive tilt
              if (tiltedOnChargerStartTime_ == 0) {
                tiltedOnChargerStartTime_ = HAL::GetTimeStamp();
              } else if (HAL::GetTimeStamp() - tiltedOnChargerStartTime_ > TILT_FAILURE_DURATION_MS) {
                // Drive forward until no tilt or timeout
                AnkiInfo( "PAP.BACKUP_ON_CHARGER.Tilted", "");
                SteeringController::ExecuteDirectDrive(40, 40);
                transitionTime_ = HAL::GetTimeStamp() + 1000;
                ProxSensors::EnableCliffDetector(true);
                mode_ = DRIVE_FORWARD;
              }
            } else if (HAL::BatteryIsOnCharger()) {
              AnkiInfo("PAP.BACKUP_ON_CHARGER.Success", "");
              SendChargerMountCompleteMessage(true);
              Reset();
            } else if (useCliffSensorAlignment_) {
              const u16 cliffBL = ProxSensors::GetCliffValue((int) HAL::CLIFF_BL);
              const u16 cliffBR = ProxSensors::GetCliffValue((int) HAL::CLIFF_BR);

              float leftSpeed  = kChargerDockingSpeedHigh;
              float rightSpeed = kChargerDockingSpeedHigh;

              const bool isBlackBL = cliffBL < kChargerCliffBlackThreshold;
              const bool isBlackBR = cliffBR < kChargerCliffBlackThreshold;

              // TODO: Only keep the logic for white stripe on dark charger once we have enough chargers (VIC-2755)
              if (chargerStripeIsBlack_) {
                // Slow down one of the sides if it's seeing black
                if (isBlackBL && !isBlackBR) {
                  leftSpeed = kChargerDockingSpeedLow;
                } else if (!isBlackBL && isBlackBR) {
                  rightSpeed = kChargerDockingSpeedLow;
                }
              } else {
                // Slow down one of the sides if it's seeing white
                if (!isBlackBL && isBlackBR) {
                  leftSpeed = kChargerDockingSpeedLow;
                } else if (isBlackBL && !isBlackBR) {
                  rightSpeed = kChargerDockingSpeedLow;
                }
              }

              SteeringController::ExecuteDirectDrive(leftSpeed, rightSpeed);
              tiltedOnChargerStartTime_ = 0;
            } else {
              tiltedOnChargerStartTime_ = 0;
            }
            break;
          }
          case DRIVE_FORWARD:
          {
            // For failed charger mounting recovery only
            if ((IMUFilter::GetPitch() > TILT_FAILURE_RECOVERY_ANGLE_RAD) ||
                (HAL::GetTimeStamp() > transitionTime_)) {
              SendChargerMountCompleteMessage(false);
              Reset();
            }
            break;
          }
          default:
          {
            Reset();
            AnkiError( "PAP.Update.InvalidAction", "%hhu", action_);
            break;
          }
        }

        return retVal;

      } // Update()

      bool IsBusy()
      {
        return mode_ != IDLE;
      }

      bool IsCarryingBlock()
      {
        return carryState_ != CarryState::CARRY_NONE;
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
                       const bool doLiftLoadCheck,
                       const f32 speed_mmps,
                       const f32 accel_mmps2,
                       const f32 decel_mmps2,
                       const f32 rel_x,
                       const f32 rel_y,
                       const f32 rel_angle,
                       const u8 numRetries)
      {
#if(DEBUG_PAP_CONTROLLER)
        AnkiDebug( "PAP.DockToBlock.Action", "%hhu", action);
#endif

        action_ = action;
        doLiftLoadCheck_ = doLiftLoadCheck;

        dockSpeed_mmps_ = speed_mmps;
        dockAccel_mmps2_ = accel_mmps2;
        dockDecel_mmps2_ = decel_mmps2;

        if (action_ == DockAction::DA_PLACE_LOW_BLIND) {
          dockOffsetDistX_ = rel_x;
          dockOffsetDistY_ = rel_y;
          dockOffsetAng_ = rel_angle;
        } else {
          dockOffsetDistX_ = 0;
          dockOffsetDistY_ = 0;
          dockOffsetAng_ = 0;
        }

        transitionTime_ = 0;
        mode_ = SET_LIFT_PREDOCK;

        DockingController::SetMaxRetries(numRetries);

      }

      void PlaceOnGround(const f32 speed_mmps,
                         const f32 accel_mmps2,
                         const f32 decel_mmps2,
                         const f32 rel_x,
                         const f32 rel_y,
                         const f32 rel_angle)
      {
        DockToBlock(DockAction::DA_PLACE_LOW_BLIND,
                    speed_mmps,
                    accel_mmps2,
                    decel_mmps2,
                    rel_x,
                    rel_y,
                    rel_angle);
      }


      void SetRollActionParams(const f32 liftHeight_mm,
                               const f32 driveSpeed_mmps,
                               const f32 driveAccel_mmps2,
                               const u32 driveDuration_ms,
                               const f32 backupDist_mm)
      {
        AnkiDebug( "PAP.SetRollActionParams", "liftHeight: %f, speed: %f, accel: %f, duration %d, backupDist %f",
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
