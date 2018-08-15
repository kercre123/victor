#include "trig_fast.h"
#include "dockingController.h"
#include "headController.h"
#include "liftController.h"
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "messages.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "localization.h"
#include "speedController.h"
#include "steeringController.h"
#include "pickAndPlaceController.h"
#include "pathFollower.h"
#include "imuFilter.h"
#include <math.h>

#define DEBUG_DOCK_CONTROLLER 0

// Resets localization pose to (0,0,0) every time a relative block pose update is received.
// Recalculates the start pose of the path that is at the same position relative to the block
// as it was when tracking was initiated. If encoder-based localization is reasonably accurate,
// this probably won't be necessary.
#define RESET_LOC_ON_BLOCK_UPDATE 0

// Limits how quickly the dockPose angle can change per docking error message received.
// The purpose being to mitigate the error introduced by rolling shutter on marker pose.
// TODO: Eventually, accurate stamping of images with time and gyro data should make it
//       possible to do rolling shutter correction on the engine. Then we can take this stuff out.
#define DOCK_ANGLE_DAMPING 0

// Whether or not to use Dubins path. If 0, just uses marker normal vector as approach path.
#define USE_DUBINS_PATH 0

// Whether or not to adjust the head angle to try to look towards the block based on our error signal
#define TRACK_BLOCK 1

// Whether or not to include the ability to do tracker docking
#define ALLOW_TRACKER_DOCKING 0


namespace Anki {
  namespace Vector {
    namespace DockingController {

      namespace {

        // Constants
        
        // Which docking method to use
        DockingMethod dockingMethod_ = DockingMethod::TRACKER_DOCKING;

        enum Mode {
          IDLE,
          LOOKING_FOR_BLOCK,
          APPROACH_FOR_DOCK
        };

#if(USE_DUBINS_PATH)
        // Turning radius of docking path
        f32 DOCK_PATH_START_RADIUS_MM = WHEEL_DIST_HALF_MM;

        // Set of radii to try when generating Dubins path to marker
        const u8 NUM_END_RADII = 3;
        f32 DOCK_PATH_END_RADII_MM[NUM_END_RADII] = {100, 40, WHEEL_DIST_HALF_MM};
#endif

        // The length of the straight tail end of the dock path.
        // Should be roughly the length of the forks on the lift.
        const f32 FINAL_APPROACH_STRAIGHT_SEGMENT_LENGTH_MM = 40;

        //const f32 FAR_DIST_TO_BLOCK_THRESH_MM = 100;

        // Distance from block face at which robot should "dock"
        f32 dockOffsetDistX_ = 0.f;
        
        // Whether or not to use only the first error signal received for docking purposes.
        // Results in smoother, albeit less accurate, docking. Useful for charger docking
        // since the marker pose estimation isn't great.
        bool useFirstErrorSignalOnly_ = false;

        TimeStamp_t lastDockingErrorSignalRecvdTime_ = 0;

        // If error signal not received in this amount of time, tracking is considered to have failed.
        const u32 STOPPED_TRACKING_TIMEOUT_MS = 500;

        // If an initial track cannot start for this amount of time, block is considered to be out of
        // view and docking is aborted.
        const u32 GIVEUP_DOCKING_TIMEOUT_MS = 1000;

#ifndef SIMULATOR
        // Compensating for motor backlash by lifting a little higher when
        // approaching an object for high pickup.
        // Lift heights are reached reasonably accurately when moving down
        // since that's how the lift is calibrated, but it tends to come
        // up a little short when approaching a target height from below.
        const f32 LIFT_HEIGHT_HIGHDOCK_PICKUP = LIFT_HEIGHT_HIGHDOCK + 3;
#else
        const f32 LIFT_HEIGHT_HIGHDOCK_PICKUP = LIFT_HEIGHT_HIGHDOCK;
#endif
        
        // Speeds and accels
        f32 dockSpeed_mmps_ = 0;
        f32 dockAccel_mmps2_ = 0;
        f32 dockDecel_mmps2_ = 0;

        // Lateral tolerance at dock pose
        // Is directly affected by camera extrinsics so it is slightly larger to try
        // to account for rotated cameras
        const f32 BLOCK_ON_GROUND_LATERAL_DOCK_TOLERANCE_MM = 3.f;
        const f32 BLOCK_NOT_ON_GROUND_LATERAL_DOCK_TOLERACNCE_MM = 5.f;
        f32 LATERAL_DOCK_TOLERANCE_AT_DOCK_MM = BLOCK_ON_GROUND_LATERAL_DOCK_TOLERANCE_MM;

        // If non-zero, once we get within this distance of the marker
        // it will continue to follow the last path and ignore timeouts.
        // Only used for high docking.
        u32 pointOfNoReturnDistMM_ = 0;

        // Whether or not the robot is currently past point of no return
        bool pastPointOfNoReturn_ = false;

        Mode mode_ = IDLE;

        // True if docking off one relative position signal
        // received via StartDockingToRelPose().
        // i.e. no vision marker required.
        bool markerlessDocking_ = false;

        // Whether or not a valid path was generated from the received error signal
        bool createdValidPath_ = false;

        // Whether or not we're already following the block surface normal as a path
        bool followingBlockNormalPath_ = false;

        // The pose of the robot at the start of docking.
        // While block tracking is maintained the robot follows
        // a path from this initial pose to the docking pose.
        Anki::Embedded::Pose2d approachStartPose_;

        // The pose of the block as we're docking
        Anki::Embedded::Pose2d blockPose_;

        // The docking pose
        Anki::Embedded::Pose2d dockPose_;

#if(DOCK_ANGLE_DAMPING)
        bool dockPoseAngleInitialized_;
#endif

#if(RESET_LOC_ON_BLOCK_UPDATE)
        // Since the physical robot currently does not localize,
        // we need to store the transform from docking pose
        // to the approachStartPose, which we then use to compute
        // a new approachStartPose with every block pose update.
        // We're faking a different approachStartPose because without
        // localization it looks like the block is moving and not the robot.

        f32 approachPath_dist, approachPath_dtheta, approachPath_dOrientation;
#endif

        // Start raising lift for high dock only when
        // the block is at least START_LIFT_TRACKING_DIST_MM close and START_LIFT_TRACKING_HEIGHT_MM high
        const f32 START_LIFT_TRACKING_DIST_MM = 90.f;

        bool dockingToBlockOnGround = true;
        const f32 DOCK_HEIGHT_LIMIT_MM = 80.f;

        // Remember the last marker pose that was fully within the
        // field of view of the camera.
        bool lastMarkerPoseObservedIsSet_ = false;
        Anki::Embedded::Pose2d lastMarkerPoseObservedInValidFOV_;

        // If the marker is out of field of view, we will continue
        // to traverse the path that was generated from that last good error signal.
        bool markerOutOfFOV_ = false;
        const f32 MARKER_WIDTH = 25.f;

        f32 headCamFOV_ver_ = 0.f;
        f32 headCamFOV_hor_ = 0.f;

        DockingErrorSignal dockingErrSignalMsg_;
        bool dockingErrSignalMsgReady_ = false;
        
        u8 numDockingFails_ = 0;
        u8 maxDockRetries_ = 2;
        Anki::Embedded::Pose2d backupPose_; // Pose of robot when it started to backup
        
        // For Hybrid Docking: the previous block pose used to determine if the block or we have moved weirdly
        // so we should acquire a new docking error signal and use it to plan a new path
        f32 prev_blockPose_x_ = -1;
        f32 prev_blockPose_y_ = -1;
        f32 prev_blockPose_a_ = -1;
        const f32 DELTA_BLOCKPOSE_X_TOL_MM = 10;
        const f32 DELTA_BLOCKPOSE_Y_TOL_MM = 10;
        const f32 DELTA_BLOCKPOSE_A_TOL_RAD = DEG_TO_RAD_F32(10);
        const u8 CONSEC_ERRSIG_TO_ACQUIRE_NEW_SIGNAL = 2;
        u8 numErrSigToAcquireNewSignal_ = 0;
       
        const u8 DIST_TO_BACKUP_ON_FAILURE_MM = 60;

#if ALLOW_TRACKER_DOCKING
        // The docking speed for the final path segment right in front of the thing we are docking with
        // Allows us to slow down when we are close to the dockPose
        const f32 finalDockSpeed_mmps_ = 20;
#endif

        // When hybrid docking the hanns maneuver tolerance for rel_x needs to be increased since we didn't push the
        // block
        const u8 HYBRID_REL_X_TOL_MM = 10;

        // If the robot's pose relative to the dockPose is more than these tolerances off it is not in position and
        // should backup and retry docking
        const u8 VERT_DOCK_TOL_MM = 11; // Distance to block in robot's x direction
        const u8 HORZ_DOCK_TOL_MM = 3; // Distance to block in robot's y direction
        const u32 TIME_SINCE_LAST_ERRSIG = 600;

        const s8 BACKUP_SPEED_MMPS = -50;
        const u8 DOCKSPEED_ON_FAILURE_MMPS = 40;
        const u8 DOCKDECEL_ON_FAILURE_MMPS2 = 40;

        // Constants for the Hanns maneuver (HM)
        const u8 HM_DIST_MM = 50;
        const f32 HM_SPEED_MMPS = 300;
        const u16 HM_ACCEL_MMPS2 = 500;
        const f32 HM_TURN_ANGLE_RAD = DEG_TO_RAD_F32(22);
        const f32 HM_ROT_SPEED_RADPS = MAX_BODY_ROTATION_SPEED_RAD_PER_SEC;
        const f32 HM_ROT_ACCEL_MMPS2 = 4.0;

        // Tolerances for doing the Hanns Maneuver when we are checking our position with the last docking error message
        const f32 ERRMSG_HM_ANGLE_TOL = DEG_TO_RAD_F32(16);
        const u8 ERRMSG_HM_LATERAL_DOCK_TOL_MM = 18;
        const u8 ERRMSG_HM_DIST_TOL = 15;
        
        // Tolerances for doing the Hanns Maneuver when we are checking our position with our pose relative to dockPose
        const u8 POSE_HM_DIST_TOL = 10;
        const u8 POSE_HM_HORZ_TOL_MM = 10;
        const f32 POSE_HM_ANGLE_TOL = DEG_TO_RAD_F32(10);
        
        // Angle tolerances for checking whether or not we are in position based on what we are using to check pose
        const f32 ERRMSG_NOT_IN_POSITION_ANGLE_TOL = DEG_TO_RAD_F32(10);
        const f32 POSE_NOT_IN_POSITION_ANGLE_TOL = DEG_TO_RAD_F32(5);
        
        // Steering controller gains for docking
        const f32 DOCKING_K1 = 0.05f;
        const f32 DOCKING_K2 = 12.f;
        const f32 DOCKING_PATH_DIST_OFFSET_CAP_MM = 20;
        const f32 DOCKING_PATH_ANG_OFFSET_CAP_RAD = 0.4f;
        bool appliedGains_ = false;
        
        // Save previous controller gains
        f32 prevK1_;
        f32 prevK2_;
        f32 prevPathDistOffsetCap_;
        f32 prevPathAngOffsetCap_;
        
#if ALLOW_TRACKER_DOCKING
        // Values related to our path to get use from our current pose to dockPose
        const u8 DIST_AWAY_FROM_BLOCK_FOR_PT_MM = 60;
        const u8 DIST_AWAY_FROM_BLOCK_FOR_ADDITIONAL_PATH_MM = 100;
        const f32 ANGLE_FROM_BLOCK_FOR_ADDITIONAL_PATH_RAD = DEG_TO_RAD_F32(25);
        const f32 ANGLE_FROM_BLOCK_FOR_POINT_TURN_RAD = DEG_TO_RAD_F32(15);
        const u8 PATH_START_DIST_BEHIND_ROBOT_MM = 60;
        const u8 MAX_DECEL_DIST_MM = 30;
#endif
        const u8 PATH_END_DIST_INTO_BLOCK_MM = 10;
        
        // If the block and cozmo are on the same plane the dock err signal z_height will be below this value
        // Normally it is ~22mm
        const u8 BLOCK_ON_GROUND_DOCK_ERR_Z_HEIGHT_MM = 25;
        
        const u8 DOCK_OFFSET_DIST_X_OFFSET = 10;
        
        // The different failure cases that can occur when cozmo is not in position during docking
        enum FailureMode {
          NO_FAILURE,
          BACKING_UP,
          HANNS_MANEUVER
        };
        FailureMode failureMode_ = NO_FAILURE;
        
        bool dockingStarted = false;
        
        DockingResult dockingResult_ = DockingResult::DOCK_UNKNOWN;
        
      } // "private" namespace

      void SetDockingMethod(DockingMethod method)
      {
        if(!IsBusy())
        {
          //AnkiDebug( "DockingController", "Setting docking method: %d", method);
          dockingMethod_ = method;
        }
      }

      bool IsBusy()
      {
        return (mode_ != IDLE);
      }

      bool DidLastDockSucceed()
      {
        if(dockingResult_ == DockingResult::DOCK_SUCCESS ||
           dockingResult_ == DockingResult::DOCK_SUCCESS_HANNS_MANEUVER ||
           dockingResult_ == DockingResult::DOCK_SUCCESS_RETRY)
        {
          return true;
        }
        return false;
      }

      f32 GetVerticalFOV() {
        AnkiConditionalError(headCamFOV_ver_ != 0.f, "DockingController.GetVerticalFOV.ZeroFOV", "");
        return headCamFOV_ver_;
      }

      f32 GetHorizontalFOV() {
        AnkiConditionalError(headCamFOV_hor_ != 0.f, "DockingController.GetHorizontalFOV.ZeroFOV", "");
        return headCamFOV_hor_;
      }
      
      DockingResult GetDockingResult()
      {
        return dockingResult_;
      }
      
      Result SendDockingStatusMessage(Status status)
      {
        DockingStatus msg;
        msg.timestamp = HAL::GetTimeStamp();
        msg.status = status;
        if(RobotInterface::SendMessage(msg)) {
          return RESULT_OK;
        }
        return RESULT_FAIL;
      }
      
      void SetMaxRetries(u8 numRetries)
      {
        maxDockRetries_ = numRetries;
      }

      // Saves previous gains and applies new docking specific ones
      void ApplyDockingSteeringGains()
      {
        if(appliedGains_)
        {
          return;
        }
        
        prevK1_ = SteeringController::GetK1Gain();
        prevK2_ = SteeringController::GetK2Gain();
        prevPathDistOffsetCap_ = SteeringController::GetPathDistOffsetCap();
        prevPathAngOffsetCap_ = SteeringController::GetPathAngOffsetCap();
        
        SteeringController::SetGains(DOCKING_K1,
                                     DOCKING_K2,
                                     DOCKING_PATH_DIST_OFFSET_CAP_MM,
                                     DOCKING_PATH_ANG_OFFSET_CAP_RAD);
        appliedGains_ = true;
      }
      
      // Sets the gains back to the original ones only if new gains were applied
      void RestoreSteeringGains()
      {
        if(appliedGains_)
        {
          SteeringController::SetGains(prevK1_, prevK2_, prevPathDistOffsetCap_, prevPathAngOffsetCap_);
          appliedGains_ = false;
        }
      }

      // Returns true if the last known pose of the marker is fully within
      // the horizontal field of view of the camera.
      bool IsMarkerInFOV(const Anki::Embedded::Pose2d &markerPose, const f32 markerWidth)
      {
        // Half fov of camera at center horizontal.
        // 0.36 radians is roughly the half-FOV of the camera bounded by the lift posts.
        f32 calcHalfFOV = 0.5f*GetHorizontalFOV();
        const f32 HALF_FOV = MIN(calcHalfFOV, 0.36f);

        const f32 markerCenterX = markerPose.GetX();
        const f32 markerCenterY = markerPose.GetY();

        // Get current robot pose
        f32 x,y;
        Radians angle;
        Localization::GetCurrPose(x, y, angle);

        // Get angle field of view edges
        Radians leftEdge = angle + HALF_FOV;
        Radians rightEdge = angle - HALF_FOV;

        // Compute angle to marker from robot
        Radians angleToMarkerCenter = atan2_fast(markerCenterY - y, markerCenterX - x);

        // Compute angle to marker edges
        // (For now, assuming marker faces the robot.)
        // TODO: Use marker angle
        f32 distToMarkerCenter = sqrtf((markerCenterY - y)*(markerCenterY - y) + (markerCenterX - x)*(markerCenterX - x));
        Radians angleToMarkerLeftEdge = angleToMarkerCenter + atan2_fast(0.5f * markerWidth, distToMarkerCenter);
        Radians angleToMarkerRightEdge = angleToMarkerCenter - atan2_fast(0.5f * markerWidth, distToMarkerCenter);


        // Check if either of the edges is outside of the fov
        f32 leftDiff = (leftEdge - angleToMarkerLeftEdge).ToFloat();
        f32 rightDiff = (rightEdge - angleToMarkerRightEdge).ToFloat();
        return (leftDiff >= 0) && (rightDiff <= 0);
      }


      Result SendGoalPoseMessage(const Anki::Embedded::Pose2d &p)
      {
        GoalPose msg;
        msg.pose.x = p.GetX();
        msg.pose.y = p.GetY();
        msg.pose.z = 0;
        msg.pose.angle = p.GetAngle().ToFloat();
        msg.followingMarkerNormal = followingBlockNormalPath_;
        if(RobotInterface::SendMessage(msg)) {
          return RESULT_OK;
        }
        return RESULT_FAIL;
      }

      // Returns the height that the lift should be moved to such that the
      // lift crossbar is just out of the field of view of the camera.
      // TODO: Should this be in some kinematics module where we have functions to get things wrt other things?
      f32 GetCamFOVLowerHeight()
      {
        f32 x, z, angle, liftH;
        HeadController::GetCamPose(x, z, angle);

        // Compute the angle of the line extending from the camera that represents
        // the lower bound of its field of view
        f32 lowerCamFOVangle = angle - 0.45f * GetVerticalFOV();

        // Compute the lift height required to raise the cross bar to be at
        // the height of that line.
        // TODO: This is really rough computation approximating with a fixed horizontal distance between
        //       the camera and the lift. make better!
        const f32 liftDistToCam = 26;
        liftH = liftDistToCam * sinf(lowerCamFOVangle) + z;
        liftH -= LIFT_XBAR_HEIGHT_WRT_WRIST_JOINT;

        //PRINT("CAM POSE: x %f, z %f, angle %f (lowerCamAngle %f, liftH %f)\n", x, z, angle, lowerCamFOVangle, liftH);

        return CLIP(liftH, LIFT_HEIGHT_LOWDOCK, LIFT_HEIGHT_CARRY);
      }


      // If docking to a high block, assumes we're trying to pick it up!
      // Gradually lift block from a height of START_LIFT_HEIGHT_MM to LIFT_HEIGHT_HIGH_DOCK
      // over the marker distance ranging from START_LIFT_TRACKING_DIST_MM to dockOffsetDistX_.
      void HighDockLiftUpdate() {
        // Don't want to move the lift while we are backing up or carrying a block
        const DockAction curAction = PickAndPlaceController::GetCurAction();
        if(failureMode_ != BACKING_UP &&
           !PickAndPlaceController::IsCarryingBlock() &&
           curAction != DockAction::DA_ALIGN &&
           curAction != DockAction::DA_ROLL_LOW &&
           curAction != DockAction::DA_DEEP_ROLL_LOW &&
           curAction != DockAction::DA_POP_A_WHEELIE &&
           curAction != DockAction::DA_ALIGN_SPECIAL)
        {
          f32 lastCommandedHeight = LiftController::GetDesiredHeight();
          if (lastCommandedHeight == dockingErrSignalMsg_.z_height) {
            // We're already at the high lift position.
            // No need to repeatedly command it.
            return;
          }
          
          // If the height of the block is higher than our normal docking height (the block is off the ground)
          // then we are doing high docking so adjust our lateral dock tolerance
          if(dockingErrSignalMsg_.z_height > BLOCK_ON_GROUND_DOCK_ERR_Z_HEIGHT_MM)
          {
            dockingToBlockOnGround = false;
            LATERAL_DOCK_TOLERANCE_AT_DOCK_MM = BLOCK_NOT_ON_GROUND_LATERAL_DOCK_TOLERACNCE_MM;
          }
          else
          {
            dockingToBlockOnGround = true;
            LATERAL_DOCK_TOLERANCE_AT_DOCK_MM = BLOCK_ON_GROUND_LATERAL_DOCK_TOLERANCE_MM;
          }

          // Compute desired slope of lift height during approach.
          // The ending lift height is made smaller the heigher the block is so that the lift doesn't rise
          // above the block which it can if just using z_height
          const f32 liftApproachSlope = (dockingErrSignalMsg_.z_height - fabsf(dockingErrSignalMsg_.z_height-BLOCK_ON_GROUND_DOCK_ERR_Z_HEIGHT_MM)/10.f) / (START_LIFT_TRACKING_DIST_MM - dockOffsetDistX_);

          // Compute current estimated distance to marker
          const f32 diffX = (dockPose_.GetX() - Localization::GetCurrPose_x());
          const f32 diffY = (dockPose_.GetY() - Localization::GetCurrPose_y());
          const f32 estDistToMarker = sqrtf(diffX * diffX + diffY * diffY);

          if (estDistToMarker < START_LIFT_TRACKING_DIST_MM) {
            // Compute current desired lift height based on current distance to block.
            f32 liftHeight = liftApproachSlope * (START_LIFT_TRACKING_DIST_MM - estDistToMarker);

            // Keep between current desired height and high dock height
            liftHeight = CLIP(liftHeight, lastCommandedHeight, LIFT_HEIGHT_HIGHDOCK_PICKUP);

            // Apply height
            LiftController::SetDesiredHeight(liftHeight);
#ifdef SIMULATOR
            // SetDesiredHeight might engage the gripper, but we don't want it engaged right now.
            HAL::DisengageGripper();
#endif
          }
        }
      }


      Result Init()
      {
        return RESULT_OK;
      }

      void SetCameraFieldOfView(f32 horizontalFOV, f32 verticalFOV)
      {
        AnkiDebug( "DockingController.SetCameraFieldOfView.Values", "H: %f, V: %f", horizontalFOV, verticalFOV);
        headCamFOV_hor_ = horizontalFOV;
        headCamFOV_ver_ = verticalFOV;
      }
      
      Result Update()
      {
      
        // If we failed to dock and are now backing up ignore error signals until we are
        // DIST_TO_BACKUP_ON_FAILURE_MM away from our initial backing up pose
        if(failureMode_ == BACKING_UP)
        {
#if(TRACK_BLOCK)
          if(dockingErrSignalMsgReady_)
          {
            // Keep looking at marker while backing up
            f32 desiredHeadAngle = atan_fast( (dockingErrSignalMsg_.z_height - NECK_JOINT_POSITION[2])/dockingErrSignalMsg_.x_distErr);
            HeadController::SetDesiredAngle(desiredHeadAngle);
          }
#endif
          
          if(Localization::GetDistTo(backupPose_.x(), backupPose_.y()) > DIST_TO_BACKUP_ON_FAILURE_MM)
          {
            failureMode_ = NO_FAILURE;
            
            // If we are doing hybrid docking then we want to backup when not in position and then fail
            // so if action retries we are close to predock pose
            if(dockingMethod_ == DockingMethod::HYBRID_DOCKING)
            {
              AnkiDebug( "DockingController.Update.BackingUpAndStopping", "");
              StopDocking(DockingResult::DOCK_FAILURE_RETRY);
            }
          }
          else
          {
            // While we are backing up we don't want to time out due to ignoring error signals so fake time
            lastDockingErrorSignalRecvdTime_ = HAL::GetTimeStamp();
            return RESULT_OK;
          }
        }
      
        // Get any docking error signal available from the vision system
        // and update our path accordingly.
        while( dockingErrSignalMsgReady_ )
        {
          dockingErrSignalMsgReady_ = false;

          // If we're not actually docking, just toss the dockingErrSignalMsg_.
          if (mode_ == IDLE) {
            break;
          }
          
          // Fail immediatly if the block height is above our docking limit
          if(dockingErrSignalMsg_.z_height > DOCK_HEIGHT_LIMIT_MM)
          {
            StopDocking(DockingResult::DOCK_FAILURE_TOO_HIGH);
            return RESULT_FAIL;
          }

#if(0)
          // Print period of tracker (i.e. messages coming in from tracker)
          static u32 lastTime = 0;
          u32 currTime = HAL::GetMicroCounter();
          if (lastTime != 0) {
            u32 period = (currTime - lastTime)/1000;
            AnkiDebug( "DockingController.Update.TrackerPeriod", "%d ms", period);
          }
          lastTime = currTime;
#endif

          // Check if we are beyond point of no return distance
          if (pastPointOfNoReturn_) {
            // If we believe we are past the point of no return distance but are actually not
            if(dockingErrSignalMsg_.x_distErr > pointOfNoReturnDistMM_)
            {
#if(DEBUG_DOCK_CONTROLLER)
              AnkiDebug( "DockingController.Update.NoLongerPastPointOfNoReturn", "(%f > %d)",
                        dockingErrSignalMsg_.x_distErr, pointOfNoReturnDistMM_);
#endif
              pastPointOfNoReturn_ = false;
            }
          }


#if(DEBUG_DOCK_CONTROLLER)
          AnkiDebug( "DockingController.Update.ReceivedErrorSignal", "time=%d, x_distErr=%f, y_horErr=%f, z_height=%f, angleErr=%fdeg",
                dockingErrSignalMsg_.timestamp,
                dockingErrSignalMsg_.x_distErr, dockingErrSignalMsg_.y_horErr,
                dockingErrSignalMsg_.z_height, RAD_TO_DEG_F32(dockingErrSignalMsg_.angleErr));
#endif

          // Check that error signal is plausible
          // If not, treat as if tracking failed.
          // TODO: Get tracker to detect these situations and not even send the error message here.
          if (dockingErrSignalMsg_.x_distErr > 0.f && ABS(dockingErrSignalMsg_.angleErr) < 0.75f*M_PI_2_F) {

            // Update time that last good error signal was received
            lastDockingErrorSignalRecvdTime_ = HAL::GetTimeStamp();

            // Set relative block pose to start/continue docking
            SetRelDockPose(dockingErrSignalMsg_.x_distErr, dockingErrSignalMsg_.y_horErr, dockingErrSignalMsg_.angleErr, dockingErrSignalMsg_.timestamp);

#if(TRACK_BLOCK)
            static const f32 MIN_HEAD_TRACKING_ANGLE = DEG_TO_RAD_F32(-20);

            f32 desiredHeadAngle = atan_fast( (dockingErrSignalMsg_.z_height - NECK_JOINT_POSITION[2])/dockingErrSignalMsg_.x_distErr) + DEG_TO_RAD_F32(4);

            // When repeatedly commanding a low head angle head calibration likes to kick in so this
            // is to prevent that from happening
            if(desiredHeadAngle > MIN_HEAD_TRACKING_ANGLE)
            {
              HeadController::SetDesiredAngle(desiredHeadAngle);
            }
#endif

            continue;
          }

          if ((!pastPointOfNoReturn_) && (!markerOutOfFOV_)) {
            SpeedController::SetUserCommandedDesiredVehicleSpeed(0);
            //PathFollower::ClearPath();
            SteeringController::ExecuteDirectDrive(0,0);
            if (mode_ != IDLE) {
              mode_ = LOOKING_FOR_BLOCK;
            }
          }

        } // while new docking error message has mail



        // Check if the pose of the marker that was in field of view should
        // again be in field of view.
        f32 distToMarker = 1000000;
        if (markerOutOfFOV_) {
          // Marker has been outside field of view.
          // Check if it should be visible again.
          if (IsMarkerInFOV(lastMarkerPoseObservedInValidFOV_, MARKER_WIDTH)) {
            AnkiDebug( "DockingController.Update.MarkerExpectedInsideFOV", "");
            // Fake the error signal received timestamp to reset the timeout
            lastDockingErrorSignalRecvdTime_ = HAL::GetTimeStamp();
            markerOutOfFOV_ = false;
          }
        } else {
          // Marker has been in field of view.
          // Check if we expect it to no longer be in view.
          if (lastMarkerPoseObservedIsSet_) {

            // Get distance between marker and current robot pose
            Embedded::Pose2d currPose = Localization::GetCurrPose();
            distToMarker = lastMarkerPoseObservedInValidFOV_.GetTranslation().Dist(currPose.GetTranslation());

            if (PathFollower::IsTraversingPath() &&
                !IsMarkerInFOV(lastMarkerPoseObservedInValidFOV_, MARKER_WIDTH - 5.f) &&
                distToMarker > FINAL_APPROACH_STRAIGHT_SEGMENT_LENGTH_MM) {
              AnkiDebug( "DockingController.Update.MarkerExpectedOutsideFOV", "");
              markerOutOfFOV_ = true;
            }
          }
        }


        // Check if we are beyond point of no return distance
        if (!pastPointOfNoReturn_ && (pointOfNoReturnDistMM_ != 0) && (distToMarker < pointOfNoReturnDistMM_)) {
#if(DEBUG_DOCK_CONTROLLER)
          AnkiDebug( "DockingController.Update.PointOfNoReturn", "(%f < %d) mode:%i", distToMarker, pointOfNoReturnDistMM_, mode_);
#endif
          pastPointOfNoReturn_ = true;
          mode_ = APPROACH_FOR_DOCK;
        }

        
        Result retVal = RESULT_OK;
        
        // There are some special cases for aligning with a block (rolling is basically aligning)
        const bool isAligning = PickAndPlaceController::GetCurAction() == DockAction::DA_ALIGN ||
        PickAndPlaceController::GetCurAction() == DockAction::DA_ROLL_LOW ||
        PickAndPlaceController::GetCurAction() == DockAction::DA_DEEP_ROLL_LOW;
        
        switch(mode_)
        {
          case IDLE:
            break;
          case LOOKING_FOR_BLOCK:

            if ((!pastPointOfNoReturn_)
                && !markerOutOfFOV_
              && (HAL::GetTimeStamp() - lastDockingErrorSignalRecvdTime_ > GIVEUP_DOCKING_TIMEOUT_MS)) {
              StopDocking(DockingResult::DOCK_FAILURE_TOO_LONG_WITHOUT_BLOCKPOSE);
              AnkiDebug( "DockingController.LOOKING_FOR_BLOCK.TooLongWithoutErrorSignal", "currTime %d, lastErrSignal %d, Giving up.", HAL::GetTimeStamp(), lastDockingErrorSignalRecvdTime_);
            }
            break;
          case APPROACH_FOR_DOCK:
          {
            HighDockLiftUpdate();

            if(dockingMethod_ != DockingMethod::EVEN_BLINDER_DOCKING)
            {
              // Stop if we haven't received error signal for a while
              if (!markerlessDocking_
                  && (!pastPointOfNoReturn_)
                  && (!useFirstErrorSignalOnly_)
                  && !markerOutOfFOV_
                  && (HAL::GetTimeStamp() - lastDockingErrorSignalRecvdTime_ > STOPPED_TRACKING_TIMEOUT_MS) ) {
                PathFollower::ClearPath();
                SpeedController::SetUserCommandedDesiredVehicleSpeed(0);
                mode_ = LOOKING_FOR_BLOCK;
                AnkiDebug( "DockingController.APPROACH_FOR_DOCK.TooLongWithoutErrorSignal", "currTime %d, lastErrSignal %d, Looking for block...", HAL::GetTimeStamp(), lastDockingErrorSignalRecvdTime_);
                break;
              }
            }
            
            // If we did the Hanns maneuver assume it succedded since we should only be doing when we
            // believe it will work
            if(failureMode_ == HANNS_MANEUVER && createdValidPath_ && !PathFollower::IsTraversingPath())
            {
              StopDocking(DockingResult::DOCK_SUCCESS_HANNS_MANEUVER);
              break;
            }
            
            // If this is DA_BACKUP_ONTO_CHARGER*, don't bother with all the checks below
            // and just assume we got to where we want. (Charger marker pose signal is
            // too noisy to trust. We just need to get roughly in front of the thing so
            // we can turn around to back into it.
            if (createdValidPath_ &&
                !PathFollower::IsTraversingPath() &&
                (PickAndPlaceController::GetCurAction() == DockAction::DA_BACKUP_ONTO_CHARGER ||
                 PickAndPlaceController::GetCurAction() == DockAction::DA_BACKUP_ONTO_CHARGER_USE_CLIFF))
            {
              StopDocking(DockingResult::DOCK_SUCCESS);
              break;
            }
            
            // If finished traversing path
            if (createdValidPath_ &&
                !PathFollower::IsTraversingPath() &&
                failureMode_ == NO_FAILURE)
            {
              // Get our current pose and compute relative x,y distances to dockPose
              Anki::Embedded::Pose2d curPose = Localization::GetCurrPose();
              Localization::ConvertToDriveCenterPose(curPose, curPose);
              Anki::Embedded::Pose2d relPose = dockPose_.GetWithRespectTo(curPose);
              f32 rel_vert_dist_block = relPose.x();
              f32 rel_horz_dist_block = relPose.y();
              
              bool inPosition = true;
              bool doHannsManeuver = false;
              
              if(dockingMethod_ != DockingMethod::BLIND_DOCKING && dockingMethod_ != DockingMethod::EVEN_BLINDER_DOCKING)
              {
                f32 rel_angle_to_block = relPose.GetAngle().ToFloat();

                // Depending on when our last docking error signal was we will either use it or the robot's relative
                // distances to the dock pose to determine if we are in position
                if(!markerlessDocking_)
                {
                  // Since hybrid docking doesn't plan into the block the error signal will be slightly larger
                  // when we finish traversing the path so increase the x_dist_err tolerance
                  f32 rel_x_tol_mm = (dockingMethod_ == DockingMethod::HYBRID_DOCKING ? HYBRID_REL_X_TOL_MM : 0);
                  if((HAL::GetTimeStamp() - dockingErrSignalMsg_.timestamp) <= TIME_SINCE_LAST_ERRSIG &&
                     (dockingErrSignalMsg_.x_distErr > pointOfNoReturnDistMM_ + rel_x_tol_mm ||
                      ABS(dockingErrSignalMsg_.y_horErr) > LATERAL_DOCK_TOLERANCE_AT_DOCK_MM ||
                      ABS(dockingErrSignalMsg_.angleErr) > ERRMSG_NOT_IN_POSITION_ANGLE_TOL))
                  {
                    AnkiDebug( "DockingController.Update.FinishedPathButErrorSignalTooLarge_1", "");
                    inPosition = false;
                    
                    // If we aren't in position but are relativly close then we want to do the Hanns maneuver
                    // The x_distErr can be fairly large due it not being that recent so reduce it by a ratio of
                    // how long ago the error was and the x_distErr at that time
                    if(dockingErrSignalMsg_.x_distErr-((HAL::GetTimeStamp()-dockingErrSignalMsg_.timestamp)/dockingErrSignalMsg_.x_distErr) <= pointOfNoReturnDistMM_+ERRMSG_HM_DIST_TOL &&
                        ABS(dockingErrSignalMsg_.y_horErr) <= ERRMSG_HM_LATERAL_DOCK_TOL_MM &&
                        ABS(dockingErrSignalMsg_.angleErr) <= ERRMSG_HM_ANGLE_TOL)
                    {
                      //AnkiDebug( "DockingController.Update.AbleToDoHannsManeuver_1", "");
  #ifndef SIMULATOR
                      doHannsManeuver = true;
  #endif
                    }
                  }
                  // If we can't use the last docking error signal then check if the robot's pose is within some
                  // tolerances of the dockPose
                  else if((HAL::GetTimeStamp() - dockingErrSignalMsg_.timestamp) > TIME_SINCE_LAST_ERRSIG &&
                          (rel_vert_dist_block > VERT_DOCK_TOL_MM ||
                           ABS(rel_horz_dist_block) > HORZ_DOCK_TOL_MM ||
                           ABS(rel_angle_to_block) > POSE_NOT_IN_POSITION_ANGLE_TOL))
                  {
                    AnkiDebug( "DockingController.Update.FinishedPathButErrorSignalTooLarge_2", "");
                    inPosition = false;
                    
                    if(rel_vert_dist_block < VERT_DOCK_TOL_MM+POSE_HM_DIST_TOL &&
                        ABS(rel_horz_dist_block) < POSE_HM_HORZ_TOL_MM &&
                        ABS(rel_angle_to_block) < POSE_HM_ANGLE_TOL)
                    {
                      //AnkiDebug( "DockingController.Update.AbleToDoHannsManeuver_2", "");
  #ifndef SIMULATOR
                      doHannsManeuver = true;
  #endif
                    }
                  }
                }
              }
              
              // If we know we are not in position and we are not currently backing up due to an already recognized
              // failure then fail this dock and either backup or do Hanns maneuver. We are only able to fail
              // docking this way if the docking involves markers
              if(!markerlessDocking_ && !inPosition)
              {
                if(dockingMethod_ != DockingMethod::BLIND_DOCKING && dockingMethod_ != DockingMethod::EVEN_BLINDER_DOCKING)
                {
                  // If we have determined we should do the Hanns maneuver and we aren't currently doing it,
                  // are docking to a block on the ground, and not carrying a block
                  if(doHannsManeuver &&
                     failureMode_ != HANNS_MANEUVER &&
                     dockingToBlockOnGround &&
                     !PickAndPlaceController::IsCarryingBlock() &&
                     !isAligning)
                  {
                    SendDockingStatusMessage(Status::STATUS_DOING_HANNS_MANEUVER);
                    AnkiDebug( "DockingController.Update.ExecutingHannsManeuver", "");
                    
                    // Hanns Maneuver was accidentally tuned to work with specific gains so apply them
                    ApplyDockingSteeringGains();
                    
                    f32 x;
                    f32 y;
                    Radians a;
                    Localization::GetDriveCenterPose(x, y, a);

                    PathFollower::ClearPath();
                    
                    f32 turnAngle = 0;
                    // Depending on which source of information we used to determine we are not in position
                    // use the same one to get the turn angle
                    if((HAL::GetTimeStamp() - dockingErrSignalMsg_.timestamp) <= TIME_SINCE_LAST_ERRSIG)
                    {
                      turnAngle = (dockingErrSignalMsg_.y_horErr >= 0 ? -HM_TURN_ANGLE_RAD : HM_TURN_ANGLE_RAD);
                    }
                    else
                    {
                      turnAngle = (rel_horz_dist_block >= 0 ? -HM_TURN_ANGLE_RAD : HM_TURN_ANGLE_RAD);
                    }
                    
                    PathFollower::AppendPathSegment_PointTurn(x, y,
                                                              a.ToFloat(),
                                                              a.ToFloat()-turnAngle,
                                                              HM_ROT_SPEED_RADPS,
                                                              HM_ROT_ACCEL_MMPS2,
                                                              HM_ROT_ACCEL_MMPS2,
                                                              0.1f, true);
                    
                    PathFollower::AppendPathSegment_Line(x,
                                                         y,
                                                         x+(HM_DIST_MM)*cosf(a.ToFloat()-turnAngle/2.f),
                                                         y+(HM_DIST_MM)*sinf(a.ToFloat()-turnAngle/2.f),
                                                         HM_SPEED_MMPS,
                                                         HM_ACCEL_MMPS2,
                                                         HM_ACCEL_MMPS2);
                    
                    createdValidPath_ = PathFollower::StartPathTraversal(0);
                    failureMode_ = HANNS_MANEUVER;
                  }
                  // Special case for aligning and docking to high block -
                  // will occur if we are in position for hanns maneuver then we won't do it
                  // and just succeed
                  else if(doHannsManeuver && (isAligning || !dockingToBlockOnGround))
                  {
                    StopDocking(DockingResult::DOCK_SUCCESS);
                  }
                  // Otherwise we are not in position and should just back up
                  else
                  {
                    SendDockingStatusMessage(Status::STATUS_BACKING_UP);
                    AnkiDebug( "DockingController.Update.BackingUp", "");
                    
                    // If we have failed too many times just give up
                    if(numDockingFails_++ >= maxDockRetries_)
                    {
                      StopDocking(DockingResult::DOCK_FAILURE_RETRY);
                      AnkiDebug( "DockingController.Update.TooManyDockingFails", "MaxRetries %d", maxDockRetries_);
                      break;
                    }
                    
                    pastPointOfNoReturn_ = false;
                    mode_ = LOOKING_FOR_BLOCK;
                    
                    backupPose_ = curPose;
                    
                    PathFollower::ClearPath();
                    createdValidPath_ = false;
                    SteeringController::ExecuteDirectDrive(BACKUP_SPEED_MMPS, BACKUP_SPEED_MMPS);
                    dockSpeed_mmps_ = DOCKSPEED_ON_FAILURE_MMPS;
                    dockDecel_mmps2_ = DOCKDECEL_ON_FAILURE_MMPS2;
                    
                    // Raise the lift if rolling, don't do anything to the lift if aligning or carrying a block, everything else
                    // we want to lower the lift when backing up
                    if(PickAndPlaceController::GetCurAction() == DockAction::DA_ROLL_LOW ||
                       PickAndPlaceController::GetCurAction() == DockAction::DA_DEEP_ROLL_LOW)
                    {
                      LiftController::SetDesiredHeight(LIFT_HEIGHT_CARRY);
                    }
                    else if(!isAligning &&
                            !PickAndPlaceController::IsCarryingBlock() &&
                            PickAndPlaceController::GetCurAction() != DockAction::DA_ALIGN_SPECIAL)
                    {
                      LiftController::SetDesiredHeightByDuration(LIFT_HEIGHT_LOWDOCK, 0.25, 0.25, 1);
                    }
                    dockingToBlockOnGround = true;
                    
                    failureMode_ = BACKING_UP;
                  }
                }
              }
              // Otherwise we are in position and docking can succeed
              else if(inPosition && failureMode_ == NO_FAILURE)
              {
                AnkiDebug( "DockingController.Update.Success", "vert:%f hort:%f rel_x:%f rel_y:%f t:%d",  rel_vert_dist_block, ABS(rel_horz_dist_block), dockingErrSignalMsg_.x_distErr, dockingErrSignalMsg_.y_horErr, (int)(HAL::GetTimeStamp()-dockingErrSignalMsg_.timestamp));
                //AnkiDebug( "DockingController.Update.DockingSucces", "");
                if(numDockingFails_ > 0)
                {
                  StopDocking(DockingResult::DOCK_SUCCESS_RETRY);
                }
                else
                {
                  StopDocking(DockingResult::DOCK_SUCCESS);
                }
              }
            }
            break;
          }
          default:
            mode_ = IDLE;
            AnkiError( "DockingController.Update.InvalidMode", "%d", mode_);
            break;
        }

        if(!DidLastDockSucceed())
        {
          retVal = RESULT_FAIL;
        }

        return retVal;

      } // Update()-


      void SetRelDockPose(f32 rel_x, f32 rel_y, f32 rel_rad, TimeStamp_t t)
      {
        // Check for readings that we do not expect to get
        if (rel_x < 0.f || ABS(rel_rad) > 0.75f*M_PI_2_F) {
          AnkiWarn( "DockingController.SetRelDockPose.OORErrorSignal", "x: %f, y: %f, rad: %f", rel_x, rel_y, rel_rad);
          return;
        }
        
        // If we are idle or already succeeded docking
        if (mode_ == IDLE || DidLastDockSucceed()) {
          // We already accomplished the dock. We're done!
          return;
        }
        
        // Ignore error signals while doing the Hanns Manuever so that we don't try to acquire a new
        // error signal because the block is moving
        if(failureMode_ == HANNS_MANEUVER)
        {
          return;
        }


#if(RESET_LOC_ON_BLOCK_UPDATE)
        // Reset localization to zero buildup of localization error.
        Localization::Init();
#endif

        // Set mode to approach if looking for a block
        if (mode_ == LOOKING_FOR_BLOCK) {

          mode_ = APPROACH_FOR_DOCK;

          // Set approach start pose
          Localization::GetDriveCenterPose(approachStartPose_.x(), approachStartPose_.y(), approachStartPose_.angle());

#if(RESET_LOC_ON_BLOCK_UPDATE)
          // If there is no localization (as is currently the case on the robot)
          // we adjust the path's starting point as the robot progresses along
          // the path so that the relative position of the starting point to the
          // block is the same as it was when tracking first started.
          approachPath_dist = sqrtf(rel_x*rel_x + rel_y*rel_y);
          approachPath_dtheta = atan2_acc(rel_y, rel_x);
          approachPath_dOrientation = rel_rad;

          //PRINT("Approach start delta: distToBlock: %f, angleToBlock: %f, blockAngleRelToRobot: %f\n", approachPath_dist,approachPath_dtheta, approachPath_dOrientation);
#endif

          followingBlockNormalPath_ = false;
        }
        
        // DOCK_OFFSET_DIST_X_OFFSET is added to the dockOffsetDistX_ here because when the robot is docked with the block
        // rel_x is generally greater than dockOffsetDistX_
        if (rel_x <= dockOffsetDistX_+DOCK_OFFSET_DIST_X_OFFSET && ABS(rel_y) <= LATERAL_DOCK_TOLERANCE_AT_DOCK_MM) {
#if(DEBUG_DOCK_CONTROLLER)
          AnkiDebug( "DockingController.SetRelDockPose.DockPoseReached", "dockOffsetDistX = %f", dockOffsetDistX_+DOCK_OFFSET_DIST_X_OFFSET);
#endif
          return;
        }

        // This error signal is with respect to the pose the robot was at at time dockMsg.timestamp
        // Find the pose the robot was at at that time and transform it to be with respect to the
        // robot's current pose
        Anki::Embedded::Pose2d histPose;
        if ((t == 0) || (t == HAL::GetTimeStamp())) {
          Localization::GetCurrPose(histPose.x(), histPose.y(), histPose.angle());
        }  else {
          Localization::GetHistPoseAtTime(t, histPose);
        }
        
#if(DEBUG_DOCK_CONTROLLER)
        Anki::Embedded::Pose2d currPose = Localization::GetCurrPose();
        AnkiDebug( "DockingController.SetRelDockPose", "HistPose %f %f %f (t=%d), currPose %f %f %f (t=%d)\n",
                  histPose.x(), histPose.y(), histPose.angle().getDegrees(), t,
                  currPose.x(), currPose.y(), currPose.angle().getDegrees(), HAL::GetTimeStamp());
#endif
        
        // Compute absolute block pose using error relative to pose at the time image was taken.
        f32 distToBlock = sqrtf((rel_x * rel_x) + (rel_y * rel_y));
        f32 angle_to_block = atan2_acc(rel_y, rel_x);
        f32 blockPose_x = histPose.x() + distToBlock * cosf(angle_to_block + histPose.GetAngle().ToFloat());
        f32 blockPose_y = histPose.y() + distToBlock * sinf(angle_to_block + histPose.GetAngle().ToFloat());
        f32 blockPose_a = (histPose.GetAngle() + rel_rad).ToFloat();
        
        // If we are hybrid docking check if blockPose has changed too much indicating
        // something moved unexpectedly
        if(dockingMethod_ == DockingMethod::HYBRID_DOCKING && prev_blockPose_x_ != -1)
        {
          if(fabsf(prev_blockPose_x_ - blockPose_x) > DELTA_BLOCKPOSE_X_TOL_MM ||
             fabsf(prev_blockPose_y_ - blockPose_y) > DELTA_BLOCKPOSE_Y_TOL_MM ||
             fabsf(prev_blockPose_a_ - blockPose_a) > DELTA_BLOCKPOSE_A_TOL_RAD)
          {
            AnkiDebug( "DockingController.SetRelDockPose.AcquireNewSignal", "%f %f %f",
                      prev_blockPose_x_ - blockPose_x,
                      prev_blockPose_y_ - blockPose_y,
                      prev_blockPose_a_ - blockPose_a);
            numErrSigToAcquireNewSignal_++;
          }
          else
          {
            numErrSigToAcquireNewSignal_ = 0;
          }
        }
        
        // Ignore error signal if blind docking or hybird docking and we dont need to acquire a new signal
        if (PathFollower::IsTraversingPath() &&
            (dockingMethod_ == DockingMethod::BLIND_DOCKING ||
             dockingMethod_ == DockingMethod::EVEN_BLINDER_DOCKING ||
             (dockingMethod_ == DockingMethod::HYBRID_DOCKING && numErrSigToAcquireNewSignal_ < CONSEC_ERRSIG_TO_ACQUIRE_NEW_SIGNAL))) {
          return;
        }
        numErrSigToAcquireNewSignal_ = 0;

        // Create new path that is aligned with the normal of the block we want to dock to.
        // End point: Where the robot origin should be by the time the robot has docked.
        // Start point: Projected from end point at specified rad.
        //              Just make length as long as distance to block.
        //
        //   ______
        //   |     |
        //   |     *  End ---------- Start              * == (rel_x, rel_y)
        //   |_____|      \ ) rad
        //    Block        \
        //                  \
        //                   \ Aligned with robot x axis (but opposite direction)
        //
        //
        //               \ +ve x axis
        //                \
        //                / ROBOT
        //               /
        //              +ve y-axis

        // Update blockPose and prevBlockPose
        blockPose_.x() = blockPose_x;
        blockPose_.y() = blockPose_y;
        blockPose_.angle() = blockPose_a;

        prev_blockPose_x_ = blockPose_.x();
        prev_blockPose_y_ = blockPose_.y();
        prev_blockPose_a_ = blockPose_.angle().ToFloat();

        // Field of view check
        if (markerOutOfFOV_) {
          // Marker has been outside field of view,
          // but the latest error signal may indicate that it is back in.
          if (IsMarkerInFOV(blockPose_, MARKER_WIDTH)) {
            //AnkiDebug( "DockingController.SetRelDockPose.MarkerInsideFOV", "");
            markerOutOfFOV_ = false;
          } else {
            //AnkiDebug( "DockingController", "Marker is expected to be out of FOV. Ignoring error signal\n");
            return;
          }
        }
        lastMarkerPoseObservedIsSet_ = true;
        lastMarkerPoseObservedInValidFOV_ = blockPose_;


#if(RESET_LOC_ON_BLOCK_UPDATE)
        // Rotate block so that it is parallel with approach start pose
        f32 rel_blockAngle = rel_rad - approachPath_dOrientation;

        // Subtract dtheta so that angle points to where start pose is
        rel_blockAngle += approachPath_dtheta;

        // Compute dx and dy from block pose in current robot frame
        f32 dx = approachPath_dist * cosf(rel_blockAngle);
        f32 dy = approachPath_dist * sinf(rel_blockAngle);

        approachStartPose_.x() = blockPose_.x() - dx;
        approachStartPose_.y() = blockPose_.y() - dy;
        approachStartPose_.angle = rel_blockAngle - approachPath_dtheta;

        //PRINT("Approach start pose: x = %f, y = %f, angle = %f\n", approachStartPose_.x(), approachStartPose_.y(), approachStartPose_.angle.ToFloat());
#endif


        // Compute dock pose
        dockPose_.x() = blockPose_.x() - dockOffsetDistX_ * cosf(blockPose_.GetAngle().ToFloat());
        dockPose_.y() = blockPose_.y() - dockOffsetDistX_ * sinf(blockPose_.GetAngle().ToFloat());
#if(DOCK_ANGLE_DAMPING)
        if (!dockPoseAngleInitialized_) {
          dockPose_.angle() = blockPose_.GetAngle();
          dockPoseAngleInitialized_ = true;
        } else {
          Radians angDiff = blockPose_.GetAngle() - dockPose_.GetAngle();
          dockPose_.angle() = dockPose_.GetAngle() + CLIP(angDiff, -0.01, 0.01);
        }
#else
        dockPose_.angle() = blockPose_.angle();
#endif

        // Send goal pose up to engine for viz
        SendGoalPoseMessage(dockPose_);

        // Convert goal pose to drive center pose for pathFollower
        Localization::ConvertToDriveCenterPose(dockPose_, dockPose_);

        // Get current drive center pose
        f32 x;
        f32 y;
        Radians a;
        Localization::GetDriveCenterPose(x, y, a);
#if(USE_DUBINS_PATH)
        s8 endRadiiIdx = -1;
        f32 path_length;
        u8 numPathSegments;
        f32 maxSegmentSweepRad = 0;
        do {
          // Clear current path
          PathFollower::ClearPath();

          if (++endRadiiIdx == NUM_END_RADII) {
            AnkiDebug( "DockingController.SetRelDockPose.DubinsPathFailed", "");
            followingBlockNormalPath_ = true;
            break;
          }

          numPathSegments = PathFollower::GenerateDubinsPath(x,
                                                             y,
                                                             a.ToFloat(),
                                                             dockPose_.x(),
                                                             dockPose_.y(),
                                                             dockPose_.GetAngle().ToFloat(),
                                                             DOCK_PATH_START_RADIUS_MM,
                                                             DOCK_PATH_END_RADII_MM[endRadiiIdx],
                                                             dockSpeed_mmps_,
                                                             dockAccel_mmps2_,
                                                             dockDecel_mmps2_,
                                                             FINAL_APPROACH_STRAIGHT_SEGMENT_LENGTH_MM,
                                                             &path_length);

          // Check if any of the arcs sweep an angle larger than PIDIV2
          maxSegmentSweepRad = 0;
          const Planning::Path& path = PathFollower::GetPath();
          for (u8 s=0; s< numPathSegments; ++s) {
            if (path.GetSegmentConstRef(s).GetType() == Planning::PST_ARC) {
              f32 segSweepRad = fabsf(path.GetSegmentConstRef(s).GetDef().arc.sweepRad);
              if (segSweepRad > maxSegmentSweepRad) {
                maxSegmentSweepRad = segSweepRad;
              }
            }
          }

        } while (numPathSegments == 0 || maxSegmentSweepRad + 0.01f >= M_PI_2_F);

        //PRINT("numPathSegments: %d, path_length: %f, distToBlock: %f, followBlockNormalPath: %d\n",
        //      numPathSegments, path_length, distToBlock, followingBlockNormalPath_);

#else
        // Skipping Dubins path since the straight line path seems to work fine as long as the steeringController gains
        // are set appropriately according to docking speed.
        followingBlockNormalPath_ = true;
#endif

        // No reasonable Dubins path exists.
        // Either try again with smaller radii or just let the controller
        // attempt to get on to a straight line normal path.
        if (followingBlockNormalPath_) {
#if ALLOW_TRACKER_DOCKING
          if(dockingMethod_ != TRACKER_DOCKING)
#endif
          {
            // Compute new starting point for path
            // HACK: Feeling lazy, just multiplying path by some scalar so that it's likely to be behind the current robot pose.
            const f32 dockPoseCos = cosf(dockPose_.GetAngle().ToFloat());
            const f32 dockPoseSin = sinf(dockPose_.GetAngle().ToFloat());
            
            f32 x_start_mm = dockPose_.x() - 3 * distToBlock * dockPoseCos;
            f32 y_start_mm = dockPose_.y() - 3 * distToBlock * dockPoseSin;
            
            // Plan just a little bit into the block to reduce the chance that we don't quite get close enough
            // and completely whiff picking up the object
            f32 distIntoBlock_mm = 5;
            
            const DockAction curAction = PickAndPlaceController::GetCurAction();
            // If we are rolling push the block a little by planning a path into it
            if(curAction == DockAction::DA_ROLL_LOW ||
               curAction == DockAction::DA_DEEP_ROLL_LOW)
            {
              distIntoBlock_mm = PATH_END_DIST_INTO_BLOCK_MM;
            }
            
            PathFollower::ClearPath();
            PathFollower::AppendPathSegment_Line(x_start_mm,
                                                 y_start_mm,
                                                 dockPose_.x() + distIntoBlock_mm*dockPoseCos,
                                                 dockPose_.y() + distIntoBlock_mm*dockPoseSin,
                                                 dockSpeed_mmps_,
                                                 dockAccel_mmps2_,
                                                 dockAccel_mmps2_);
            
            //AnkiDebug( "DockingController", "Computing straight line path (%f, %f) to (%f, %f)\n",x_start_mm, y_start_mm, dockPose_.x(), dockPose_.y());
          }
#if ALLOW_TRACKER_DOCKING
          else
          {
            // Distance it takes in order to decelerate from dockSpeed to 0 with the given dock deceleration
            // This is for the final path segment so we get a nice smooth ease-in when close to the block
            f32 distToDecel = (-(dockSpeed_mmps_*dockSpeed_mmps_)) / (-2*dockDecel_mmps2_);
            
            // If distToDecel is larger than our max decel distance calculate a new deceleration such that distToDecel will
            // be equivalent to our max decel distance
            if(distToDecel > MAX_DECEL_DIST_MM)
            {
              dockDecel_mmps2_ = (-(dockSpeed_mmps_*dockSpeed_mmps_)) / (-2*MAX_DECEL_DIST_MM);
              distToDecel = MAX_DECEL_DIST_MM;
            }
            
            
            // If we need to do a point turn in order to turn towards the block than this padding will ensure we are
            // DIST_AWAY_FROM_BLOCK_FOR_PT_MM away from the block when we turn.
            // This is so that we don't turn and collide with the block
            f32 distToDecelPadding = 0;
            if(distToDecel < DIST_AWAY_FROM_BLOCK_FOR_PT_MM && distToBlock > DIST_AWAY_FROM_BLOCK_FOR_PT_MM)
            {
              distToDecelPadding = DIST_AWAY_FROM_BLOCK_FOR_PT_MM - distToDecel;
            }
            
            f32 dockPoseAngleCos = cosf(dockPose_.GetAngle().ToFloat());
            f32 dockPoseAngleSin = sinf(dockPose_.GetAngle().ToFloat());
            
            f32 x_inFrontOfBlock = dockPose_.x()-(distToDecelPadding + distToDecel)*dockPoseAngleCos;
            f32 y_inFrontOfBlock = dockPose_.y()-(distToDecelPadding + distToDecel)*dockPoseAngleSin;
            
            // Make sure our starting point is behind us
            x -= PATH_START_DIST_BEHIND_ROBOT_MM*dockPoseAngleCos;
            y -= PATH_START_DIST_BEHIND_ROBOT_MM*dockPoseAngleSin;

            PathFollower::ClearPath();
            
            // If we are more than DIST_AWAY_FROM_BLOCK_FOR_ADDITIONAL_PATH_MM away from the block or the
            // blocks angle reletive to robot is greater than ANGLE_FROM_BLOCK_FOR_ADDITIONAL_PATH_RAD plan
            // a path to the point DIST_AWAY_FROM_BLOCK_FOR_PT_MM directly in front of the block
            if(distToBlock > DIST_AWAY_FROM_BLOCK_FOR_ADDITIONAL_PATH_MM ||
               ABS(rel_rad) > ANGLE_FROM_BLOCK_FOR_ADDITIONAL_PATH_RAD)
            {
              PathFollower::AppendPathSegment_Line(0,
                                                   x,
                                                   y,
                                                   x_inFrontOfBlock,
                                                   y_inFrontOfBlock,
                                                   dockSpeed_mmps_,
                                                   dockAccel_mmps2_,
                                                   dockDecel_mmps2_);
              
              // If the angle of the block relative to the robot is greater than ANGLE_FROM_BLOCK_FOR_POINT_TURN_RAD
              // we will want to do a point turn in order face the block
              if(ABS(rel_rad) > ANGLE_FROM_BLOCK_FOR_POINT_TURN_RAD)
              {
                PathFollower::AppendPathSegment_PointTurn(0,
                                                          x_inFrontOfBlock,
                                                          y_inFrontOfBlock,
                                                          dockPose_.GetAngle().ToFloat(), 3.0, 1.0, 1.0, 0.1, true);
              }
              
              // If we added padding to place us 60mm away from the block then we need to add a path segment to get us
              // from that point to the point where we will start to decelerate otherwise we will already be at
              // the point of deceleration
              if(distToDecelPadding > 0)
              {
                PathFollower::AppendPathSegment_Line(0,
                                                     x_inFrontOfBlock,
                                                     y_inFrontOfBlock,
                                                     dockPose_.x()-(distToDecel)*dockPoseAngleCos,
                                                     dockPose_.y()-(distToDecel)*dockPoseAngleSin,
                                                     dockSpeed_mmps_,
                                                     dockAccel_mmps2_,
                                                     dockDecel_mmps2_);
              }
            }
            // Else we are close enough and roughly directly in front of the block that we can just go straight
            // to the deceleration point
            else if(distToBlock > distToDecel)
            {
              PathFollower::AppendPathSegment_Line(0,
                                                   x,
                                                   y,
                                                   dockPose_.x()-(distToDecel)*dockPoseAngleCos,
                                                   dockPose_.y()-(distToDecel)*dockPoseAngleSin,
                                                   dockSpeed_mmps_,
                                                   dockAccel_mmps2_,
                                                   dockDecel_mmps2_);
            }
            
            // Finally add a segment from our deceleration point to a point PATH_END_DIST_INTO_BLOCK_MM
            // (or if docking to a block off the ground 0mm) inside of the block
            // Placing the point inside the block causes us to push the block and maybe it will slide sideways onto
            // the lift if we are ever so slightly off the marker due to camera extrinsics
            f32 distIntoBlock = ((!dockingToBlockOnGround ||
                                  PickAndPlaceController::IsCarryingBlock() ||
                                  PickAndPlaceController::GetCurAction() == DA_ALIGN) ? 0 : PATH_END_DIST_INTO_BLOCK_MM);
            
            PathFollower::AppendPathSegment_Line(0,
                                                 dockPose_.x()-(distToDecel)*dockPoseAngleCos,
                                                 dockPose_.y()-(distToDecel)*dockPoseAngleSin,
                                                 dockPose_.x()+(distIntoBlock)*dockPoseAngleCos,
                                                 dockPose_.y()+(distIntoBlock)*dockPoseAngleSin,
                                                 finalDockSpeed_mmps_,
                                                 dockAccel_mmps2_,
                                                 dockDecel_mmps2_);
          }
#endif
        }

        // Start following path
        createdValidPath_ = PathFollower::StartPathTraversal(0);

        // Debug
        if (!createdValidPath_) {
          AnkiError( "DockingController.SetRelDockPose.FailedToCreatePath", "");
          PathFollower::PrintPath();
        }

      }

      void StartDocking_internal(const f32 speed_mmps, const f32 accel_mmps, const f32 decel_mmps)
      {
        dockingStarted = true;
        dockingResult_ = DockingResult::DOCK_UNKNOWN;
      
        dockSpeed_mmps_ = speed_mmps;
        dockAccel_mmps2_ = accel_mmps;
        dockDecel_mmps2_ = decel_mmps;
        mode_ = LOOKING_FOR_BLOCK;
        lastDockingErrorSignalRecvdTime_ = HAL::GetTimeStamp();
        numErrSigToAcquireNewSignal_ = 0;
        
        // Change gains for tracker docking to smooth pathfollowing due to constantly changing path
        if(dockingMethod_ == DockingMethod::TRACKER_DOCKING)
        {
          ApplyDockingSteeringGains();
        }
      }

      void StartDocking(const f32 speed_mmps, const f32 accel_mmps, const f32 decel_mmps,
                        const f32 dockOffsetDistX, const f32 dockOffsetDistY, const f32 dockOffsetAngle,
                        const u32 pointOfNoReturnDistMM,
                        const bool useFirstErrSignalOnly)
      {
        StartDocking_internal(speed_mmps, accel_mmps, decel_mmps);
        
        dockOffsetDistX_ = dockOffsetDistX;
        pointOfNoReturnDistMM_ = pointOfNoReturnDistMM;
        pastPointOfNoReturn_ = false;
        dockingToBlockOnGround = true;
        markerlessDocking_ = false;
        useFirstErrorSignalOnly_ = useFirstErrSignalOnly;

#if(DOCK_ANGLE_DAMPING)
        dockPoseAngleInitialized_ = false;
#endif
      }

      void StartDockingToRelPose(const f32 speed_mmps, const f32 accel_mmps, const f32 decel_mmps,
                                 const f32 rel_x, const f32 rel_y, const f32 rel_angle)
      {
        StartDocking_internal(speed_mmps, accel_mmps, decel_mmps);

        markerlessDocking_ = true;

        // NOTE: mode_ must be set to LOOKING_FOR_BLOCK and success_ must be false
        //       before we call SetRelDockPose()
        SetRelDockPose(rel_x, rel_y, rel_angle);
      }

      void StopDocking(DockingResult result) {
        if(!dockingStarted)
        {
          return;
        }
        
        dockingResult_ = result;
      
        SpeedController::SetUserCommandedDesiredVehicleSpeed(0);
        PathFollower::ClearPath();
        SteeringController::ExecuteDirectDrive(0,0);
        mode_ = IDLE;

        pointOfNoReturnDistMM_ = 0;
        pastPointOfNoReturn_ = false;
        markerlessDocking_ = false;
        markerOutOfFOV_ = false;
        lastMarkerPoseObservedIsSet_ = false;
        dockingToBlockOnGround = false;
        useFirstErrorSignalOnly_ = false;
        numDockingFails_ = 0;
        LATERAL_DOCK_TOLERANCE_AT_DOCK_MM = BLOCK_ON_GROUND_LATERAL_DOCK_TOLERANCE_MM;
        
        failureMode_ = NO_FAILURE;
        
        prev_blockPose_x_ = -1;
        prev_blockPose_y_ = -1;
        prev_blockPose_a_ = -1;
        
        RestoreSteeringGains();
        
        dockingStarted = false;
      }

      const Anki::Embedded::Pose2d& GetLastMarkerAbsPose()
      {
        return blockPose_;
      }

      f32 GetDistToLastDockMarker()
      {        
        // Get distance to last marker location
        f32 dx = blockPose_.x() - Localization::GetCurrPose_x();
        f32 dy = blockPose_.y() - Localization::GetCurrPose_y();
        f32 dist = sqrtf(dx*dx + dy*dy);
        return dist;
      }

      void SetDockingErrorSignalMessage(const DockingErrorSignal& msg)
      {
        // If the path is already going then ignore any incoming error signals if
        // only using the first error signal.
        if (useFirstErrorSignalOnly_ && PathFollower::IsTraversingPath()) {
          return;
        }
        
        dockingErrSignalMsg_ = msg;
        dockingErrSignalMsgReady_ = true;
      }

      } // namespace DockingController
    } // namespace Vector
  } // namespace Anki
