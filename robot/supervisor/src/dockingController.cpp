#include "anki/common/robot/config.h"
#include "anki/common/robot/trig_fast.h"
#include "dockingController.h"
#include "headController.h"
#include "liftController.h"

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/common/robot/geometry.h"
#include "anki/cozmo/robot/hal.h"
#include "localization.h"
#include "speedController.h"
#include "steeringController.h"
#include "pathFollower.h"


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
#define DOCK_ANGLE_DAMPING 1

namespace Anki {
  namespace Cozmo {
    namespace DockingController {
      
      namespace {
        
        // Constants
        
        enum Mode {
          IDLE,
          LOOKING_FOR_BLOCK,
          APPROACH_FOR_DOCK
        };

        // Turning radius of docking path
        f32 DOCK_PATH_START_RADIUS_MM = WHEEL_DIST_HALF_MM;
        
        // Set of radii to try when generating Dubins path to marker
        const u8 NUM_END_RADII = 3;
        f32 DOCK_PATH_END_RADII_MM[NUM_END_RADII] = {100, 40, WHEEL_DIST_HALF_MM};
        
        // The length of the straight tail end of the dock path.
        // Should be roughly the length of the forks on the lift.
        const f32 FINAL_APPROACH_STRAIGHT_SEGMENT_LENGTH_MM = 40;

        //const f32 FAR_DIST_TO_BLOCK_THRESH_MM = 100;
        
        // Distance from block face at which robot should "dock"
        f32 dockOffsetDistX_ = 0.f;
        
        TimeStamp_t lastDockingErrorSignalRecvdTime_ = 0;
        
        // If error signal not received in this amount of time, tracking is considered to have failed.
        const u32 STOPPED_TRACKING_TIMEOUT_MS = 500;
        
        // If an initial track cannot start for this amount of time, block is considered to be out of
        // view and docking is aborted.
        const u32 GIVEUP_DOCKING_TIMEOUT_MS = 1000;
        
        const u16 DOCK_APPROACH_SPEED_MMPS = 50;
        //const u16 DOCK_FAR_APPROACH_SPEED_MMPS = 30;
        const u16 DOCK_APPROACH_ACCEL_MMPS2 = 200;
        const u16 DOCK_APPROACH_DECEL_MMPS2 = 200;
        
        // Lateral tolerance at dock pose
        const f32 LATERAL_DOCK_TOLERANCE_AT_DOCK_MM = 1.75f;
        
        // If non-zero, once we get within this distance of the marker
        // it will continue to follow the last path and ignore timeouts.
        // Only used for high docking.
        u32 pointOfNoReturnDistMM_ = 0;
        
        // Whether or not the robot is currently past point of no return
        bool pastPointOfNoReturn_ = false;

        Mode mode_ = IDLE;
        
        // Whether or not the last docking attempt succeeded
        bool success_  = false;

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
        
        // Whether or not docking path should be traversed with manually controlled speed
        bool useManualSpeed_ = false;
        
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
        const f32 START_LIFT_TRACKING_DIST_MM = 80.f;
        const f32 START_LIFT_TRACKING_HEIGHT_MM = 44.f;
        
        // First commanded lift height when START_LIFT_TRACKING_DIST_MM is reached
        const f32 START_LIFT_HEIGHT_MM = LIFT_HEIGHT_HIGHDOCK - 15.f;

        // Whether or not to raise lift in prep for high docking
        bool doHighDockLiftTracking_ = false;
        
        // Last received docking error
        f32 lastMarkerDistX_ = 0.f;
        f32 lastMarkerDistY_ = 0.f;
        f32 lastMarkerAng_ = 0.f;

        // Remember the last marker pose that was fully within the
        // field of view of the camera.
        bool lastMarkerPoseObservedIsSet_ = false;
        Anki::Embedded::Pose2d lastMarkerPoseObservedInValidFOV_;
        
        // If the marker is out of field of view, we will continue
        // to traverse the path that was generated from that last good error signal.
        bool markerOutOfFOV_ = false;
        const f32 MARKER_WIDTH = 25.f;
        
        f32 headCamFOV_ver_;
        f32 headCamFOV_hor_;
        
        Messages::DockingErrorSignal dockingErrSignalMsg_;
        bool dockingErrSignalMsgReady_ = false;
        
      } // "private" namespace
      
      bool IsBusy()
      {
        return (mode_ != IDLE);
      }
      
      bool DidLastDockSucceed()
      {
        return success_;
      }
      
      f32 GetVerticalFOV() {
        return headCamFOV_ver_;
      }
      
      f32 GetHorizontalFOV() {
        return headCamFOV_hor_;
      }

      
      // Returns true if the last known pose of the marker is fully within
      // the horizontal field of view of the camera.
      bool IsMarkerInFOV(const Anki::Embedded::Pose2d &markerPose, const f32 markerWidth)
      {
        // Half fov of camera at center horizontal.
        // 0.36 radians is roughly the half-FOV of the camera bounded by the lift posts.
        const f32 HALF_FOV = MIN(0.5*GetHorizontalFOV(), 0.36);
        
        const f32 markerCenterX = markerPose.GetX();
        const f32 markerCenterY = markerPose.GetY();
        
        // Get current robot pose
        f32 x,y;
        Radians angle;
        Localization::GetCurrentMatPose(x, y, angle);
        
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
        Messages::GoalPose msg;
        msg.pose_x = p.GetX();
        msg.pose_y = p.GetY();
        msg.pose_z = 0;
        msg.pose_angle = p.GetAngle().ToFloat();
        msg.followingMarkerNormal = followingBlockNormalPath_;
        if(HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::GoalPose), &msg)) {
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
        if (doHighDockLiftTracking_) {
          
          f32 lastCommandedHeight = LiftController::GetDesiredHeight();
          if (lastCommandedHeight == LIFT_HEIGHT_HIGHDOCK) {
            // We're already at the high lift position.
            // No need to repeatedly command it.
            return;
          }
          
          // Compute desired slope of lift height during approach.
          const f32 liftApproachSlope = (LIFT_HEIGHT_HIGHDOCK - START_LIFT_HEIGHT_MM) / (START_LIFT_TRACKING_DIST_MM - dockOffsetDistX_);
          
          // Compute current estimated distance to marker
          f32 robotX, robotY;
          Radians robotAngle;
          Localization::GetCurrentMatPose(robotX, robotY, robotAngle);
          
          f32 diffX = (blockPose_.GetX() - robotX);
          f32 diffY = (blockPose_.GetY() - robotY);
          f32 estDistToMarker = sqrtf(diffX * diffX + diffY * diffY);

          
          if (estDistToMarker < START_LIFT_TRACKING_DIST_MM) {
            
            // Compute current desired lift height based on current distance to block.
            f32 liftHeight = START_LIFT_HEIGHT_MM + liftApproachSlope * (START_LIFT_TRACKING_DIST_MM - estDistToMarker);
            
            // Keep between current desired height and high dock height
            liftHeight = CLIP(liftHeight, lastCommandedHeight, LIFT_HEIGHT_HIGHDOCK);
            
            // Apply height
            LiftController::SetDesiredHeight(liftHeight);
          }
        }

      }
      
      
      Result Init()
      {
        const HAL::CameraInfo* headCamInfo = HAL::GetHeadCamInfo();

        AnkiConditionalErrorAndReturnValue(headCamInfo != NULL, RESULT_FAIL_INVALID_OBJECT,
                                           "DockingController::Init()",
                                           "NULL head cam info!\n");
        
        // Compute FOV from focal length (currently used for tracker prediciton)
        headCamFOV_ver_ = 2.f * atanf(static_cast<f32>(headCamInfo->nrows) /
                                      (2.f * headCamInfo->focalLength_y));
        headCamFOV_hor_ = 2.f * atanf(static_cast<f32>(headCamInfo->ncols) /
                                      (2.f * headCamInfo->focalLength_x));

        return RESULT_OK;
      }
      
      
      Result Update()
      {
        
        // Get any docking error signal available from the vision system
        // and update our path accordingly.
        while( dockingErrSignalMsgReady_ )
        {
          dockingErrSignalMsgReady_ = false;
          
          // If we're not actually docking, just toss the dockingErrSignalMsg_.
          if (mode_ == IDLE) {
            break;
          }
          
#if(0)
          // Print period of tracker (i.e. messages coming in from tracker)
          static u32 lastTime = 0;
          u32 currTime = HAL::GetMicroCounter();
          if (lastTime != 0) {
            u32 period = (currTime - lastTime)/1000;
            PRINT("PERIOD: %d ms\n", period);
          }
          lastTime = currTime;
#endif
          
          //PRINT("ErrSignal %d (msgTime %d)\n", HAL::GetMicroCounter(), dockingErrSignalMsg_.timestamp);
          
          // Update last observed marker pose
          lastMarkerDistX_ = dockingErrSignalMsg_.x_distErr;
          lastMarkerDistY_ = dockingErrSignalMsg_.y_horErr;
          lastMarkerAng_ = dockingErrSignalMsg_.angleErr;
          
          
          // Check if we are beyond point of no return distance
          if (pastPointOfNoReturn_) {
#if(DEBUG_DOCK_CONTROLLER)
            PRINT("DockingController: Ignoring error msg because past point of no return (%f < %d)\n", dockingErrSignalMsg_.x_distErr, pointOfNoReturnDistMM_);
#endif
            break; // out of while
          }
          
          
#if(DEBUG_DOCK_CONTROLLER)
          PRINT("Received%sdocking error signal: x_distErr=%f, y_horErr=%f, "
                "z_height=%f, angleErr=%fdeg\n",
                (dockMsg.isApproximate ? " approximate " : " "),
                dockingErrSignalMsg_.x_distErr, dockingErrSignalMsg_.y_horErr,
                dockingErrSignalMsg_.z_height, RAD_TO_DEG_F32(dockingErrSignalMsg_.angleErr));
#endif
          
          // Check that error signal is plausible
          // If not, treat as if tracking failed.
          // TODO: Get tracker to detect these situations and not even send the error message here.
          if (dockingErrSignalMsg_.x_distErr > 0.f && ABS(dockingErrSignalMsg_.angleErr) < 0.75f*PIDIV2_F) {
            
            // Update time that last good error signal was received
            lastDockingErrorSignalRecvdTime_ = HAL::GetTimeStamp();
            
            // Set relative block pose to start/continue docking
            SetRelDockPose(dockingErrSignalMsg_.x_distErr, dockingErrSignalMsg_.y_horErr, dockingErrSignalMsg_.angleErr, dockingErrSignalMsg_.timestamp);
            
            // If we have the height of the marker for docking, we can also
            // compute the head angle to keep it centered
            //HeadController::SetMaxSpeedAndAccel(2.5, 10);
            //f32 desiredHeadAngle = atan_fast( (dockMsg.z_height - NECK_JOINT_POSITION[2])/dockMsg.x_distErr);
            
            // Make sure bottom of camera FOV doesn't tilt below the bottom of the block
            // or that the camera FOV center doesn't tilt below the marker center.
            // Otherwise try to maintain the lowest tilt possible
            
            // Compute angle the head needs to face such that the bottom of the marker
            // is at the bottom of the image.
            //f32 minDesiredHeadAngle1 = atan_fast( (dockMsg.z_height - NECK_JOINT_POSITION[2] - 20.f)/dockMsg.x_distErr) + 0.5f*GetVerticalFOV(); // TODO: Marker size should come from VisionSystem?
            
            // Compute the angle the head needs to face such that it is looking
            // directly at the center of the marker
            //f32 minDesiredHeadAngle2 = atan_fast( (dockMsg.z_height - NECK_JOINT_POSITION[2])/dockMsg.x_distErr);
            
            // Use the min of both angles
            //f32 desiredHeadAngle = MIN(minDesiredHeadAngle1, minDesiredHeadAngle2);
            
            // KEVIN: Lens is wide enough now that we don't really need to do head tracking.
            //        Docking is smoother without it!
            //HeadController::SetDesiredAngle(desiredHeadAngle);
            //PRINT("desHeadAngle %f (min1: %f, min2: %f)\n", desiredHeadAngle, minDesiredHeadAngle1, minDesiredHeadAngle2);
            
            // If docking to a high block, assumes we're trying to pick it up!
            if (dockingErrSignalMsg_.z_height > START_LIFT_TRACKING_HEIGHT_MM) {
              doHighDockLiftTracking_ = true;
            }
            
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
          
        } // while dockErrSignalMailbox has mail

        
        
        // Check if the pose of the marker that was in field of view should
        // again be in field of view.
        f32 distToMarker = 1000000;
        if (markerOutOfFOV_) {
          // Marker has been outside field of view.
          // Check if it should be visible again.
          if (IsMarkerInFOV(lastMarkerPoseObservedInValidFOV_, MARKER_WIDTH)) {
            PRINT("Marker should be INSIDE FOV\n");
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
            distToMarker = lastMarkerPoseObservedInValidFOV_.get_xy().Dist(currPose.get_xy());
            
            if (PathFollower::IsTraversingPath() &&
                !IsMarkerInFOV(lastMarkerPoseObservedInValidFOV_, MARKER_WIDTH - 5.f) &&
                distToMarker > FINAL_APPROACH_STRAIGHT_SEGMENT_LENGTH_MM) {
              PRINT("Marker should be OUTSIDE FOV\n");
              markerOutOfFOV_ = true;
            }
          }
        }

        
        // Check if we are beyond point of no return distance
        if (!pastPointOfNoReturn_ && (pointOfNoReturnDistMM_ != 0) && (distToMarker < pointOfNoReturnDistMM_)) {
#if(DEBUG_DOCK_CONTROLLER)
          PRINT("DockingController: Point of no return (%f < %d)\n", distToMarker, pointOfNoReturnDistMM_);
#endif
          pastPointOfNoReturn_ = true;
          mode_ = APPROACH_FOR_DOCK;
        }

        
        
        Result retVal = RESULT_OK;
        
        switch(mode_)
        {
          case IDLE:
            break;
          case LOOKING_FOR_BLOCK:
            
            if ((!pastPointOfNoReturn_)
                && !markerOutOfFOV_
              && (HAL::GetTimeStamp() - lastDockingErrorSignalRecvdTime_ > GIVEUP_DOCKING_TIMEOUT_MS)) {
              ResetDocker();
#if(DEBUG_DOCK_CONTROLLER)
              PRINT("Too long without block pose (currTime %d, lastErrSignal %d). Giving up.\n", HAL::GetTimeStamp(), lastDockingErrorSignalRecvdTime_);
#endif
            }
            break;
          case APPROACH_FOR_DOCK:
          {
            HighDockLiftUpdate();
            
            // Stop if we haven't received error signal for a while
            if (!markerlessDocking_
                && (!pastPointOfNoReturn_)
                && !markerOutOfFOV_
                && (HAL::GetTimeStamp() - lastDockingErrorSignalRecvdTime_ > STOPPED_TRACKING_TIMEOUT_MS) ) {
              PathFollower::ClearPath();
              SpeedController::SetUserCommandedDesiredVehicleSpeed(0);
              mode_ = LOOKING_FOR_BLOCK;
#if(DEBUG_DOCK_CONTROLLER)
              PRINT("Too long without block pose (currTime %d, lastErrSignal %d). Looking for block...\n", HAL::GetTimeStamp(), lastDockingErrorSignalRecvdTime_);
#endif
              break;
            }
            
            // If finished traversing path
            if (createdValidPath_ && !PathFollower::IsTraversingPath()) {
#if(DEBUG_DOCK_CONTROLLER)
              PRINT("*** DOCKING SUCCESS ***\n");
#endif
              ResetDocker();
              success_ = true;
              
              // If we successfully reached end of path, set lastMarkerDistX to reflect that.
              lastMarkerDistX_ = ORIGIN_TO_LOW_LIFT_DIST_MM; // This depends on what action we were doing, but it's good enough for all of them.
              lastMarkerDistY_ = 0;
              
              break;
            }
            
            break;
          }
          default:
            mode_ = IDLE;
            success_ = false;
            PRINT("Reached default case in DockingController "
                  "mode switch statement.(1)\n");
            break;
        }
        

        if(success_ == false)
        {
          retVal = RESULT_FAIL;
        }
        
        return retVal;
        
      } // Update()
      
      
      void SetRelDockPose(f32 rel_x, f32 rel_y, f32 rel_rad, TimeStamp_t t)
      {
        // Check for readings that we do not expect to get
        if (rel_x < 0.f || ABS(rel_rad) > 0.75f*PIDIV2_F
            ) {
          PRINT("WARN: Ignoring out of range docking error signal (%f, %f, %f)\n", rel_x, rel_y, rel_rad);
          return;
        }
        
        if (mode_ == IDLE || success_) {
          // We already accomplished the dock. We're done!
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
          Localization::GetDriveCenterPose(approachStartPose_.x(), approachStartPose_.y(), approachStartPose_.angle);
          
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
        
        if (rel_x <= dockOffsetDistX_ && ABS(rel_y) < LATERAL_DOCK_TOLERANCE_AT_DOCK_MM) {
#if(DEBUG_DOCK_CONTROLLER)
          PRINT("DOCK POSE REACHED (dockOffsetDistX = %f)\n", dockOffsetDistX_);
#endif
          return;
        }
        

        // This error signal is with respect to the pose the robot was at at time dockMsg.timestamp
        // Find the pose the robot was at at that time and transform it to be with respect to the
        // robot's current pose
        Anki::Embedded::Pose2d histPose;
        if ((t == 0) || (t == HAL::GetTimeStamp())) {
          Localization::GetCurrentMatPose(histPose.x(), histPose.y(), histPose.angle);
        }  else {
          Localization::GetHistPoseAtTime(t, histPose);
        }
        
#if(DEBUG_DOCK_CONTROLLER)
        Anki::Embedded::Pose2d currPose;
        Localization::GetCurrentMatPose(currPose.x(), currPose.y(), currPose.angle);
        PRINT("HistPose %f %f %f (t=%d), currPose %f %f %f (t=%d)\n",
              histPose.x(), histPose.y(), histPose.angle.getDegrees(), t,
              currPose.x(), currPose.y(), currPose.angle.getDegrees(), HAL::GetTimeStamp());
#endif
        
        // Compute absolute block pose using error relative to pose at the time image was taken.
        f32 distToBlock = sqrtf((rel_x * rel_x) + (rel_y * rel_y));
        f32 rel_angle_to_block = atan2_acc(rel_y, rel_x);
        blockPose_.x() = histPose.x() + distToBlock * cosf(rel_angle_to_block + histPose.angle.ToFloat());
        blockPose_.y() = histPose.y() + distToBlock * sinf(rel_angle_to_block + histPose.angle.ToFloat());
        blockPose_.angle = histPose.angle + rel_rad;

        
        // Field of view check
        if (markerOutOfFOV_) {
          // Marker has been outside field of view,
          // but the latest error signal may indicate that it is back in.
          if (IsMarkerInFOV(blockPose_, MARKER_WIDTH)) {
            PRINT("Marker signal is INSIDE FOV\n");
            markerOutOfFOV_ = false;
          } else {
            //PRINT("Marker is expected to be out of FOV. Ignoring error signal\n");
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
        dockPose_.x() = blockPose_.x() - dockOffsetDistX_ * cosf(blockPose_.angle.ToFloat());
        dockPose_.y() = blockPose_.y() - dockOffsetDistX_ * sinf(blockPose_.angle.ToFloat());
#if(DOCK_ANGLE_DAMPING)
        if (!dockPoseAngleInitialized_) {
          dockPose_.angle = blockPose_.angle;
          dockPoseAngleInitialized_ = true;
        } else {
          Radians angDiff = blockPose_.angle - dockPose_.angle;
          dockPose_.angle = dockPose_.angle + CLIP(angDiff, -0.01, 0.01);
        }
#else
        dockPose_.angle = blockPose_.angle;
#endif
        
        // Send goal pose up to engine for viz
        SendGoalPoseMessage(dockPose_);

        // Convert goal pose to drive center pose for pathFollower
        Localization::ConvertToDriveCenterPose(dockPose_, dockPose_);
        
        

        s8 endRadiiIdx = -1;
        f32 path_length;
        u8 numPathSegments;
        f32 maxSegmentSweepRad = 0;
        do {
          // Clear current path
          PathFollower::ClearPath();
         
          if (++endRadiiIdx == NUM_END_RADII) {
            //PRINT("Could not generate Dubins path\n");
            followingBlockNormalPath_ = true;
            break;
          }
          
          numPathSegments = PathFollower::GenerateDubinsPath(approachStartPose_.x(),
                                                             approachStartPose_.y(),
                                                             approachStartPose_.angle.ToFloat(),
                                                             dockPose_.x(),
                                                             dockPose_.y(),
                                                             dockPose_.angle.ToFloat(),
                                                             DOCK_PATH_START_RADIUS_MM,
                                                             DOCK_PATH_END_RADII_MM[endRadiiIdx],
                                                             DOCK_APPROACH_SPEED_MMPS,
                                                             DOCK_APPROACH_ACCEL_MMPS2,
                                                             DOCK_APPROACH_DECEL_MMPS2,
                                                             FINAL_APPROACH_STRAIGHT_SEGMENT_LENGTH_MM,
                                                             &path_length);
          
          // Check if any of the arcs sweep an angle larger than PIDIV2
          maxSegmentSweepRad = 0;
          const Planning::Path& path = PathFollower::GetPath();
          for (u8 s=0; s< numPathSegments; ++s) {
            if (path.GetSegmentConstRef(s).GetType() == Planning::PST_ARC) {
              f32 segSweepRad = ABS(path.GetSegmentConstRef(s).GetDef().arc.sweepRad);
              if (segSweepRad > maxSegmentSweepRad) {
                maxSegmentSweepRad = segSweepRad;
              }
            }
          }
          
        } while (numPathSegments == 0 || maxSegmentSweepRad + 0.01f >= PIDIV2_F);

        //PRINT("numPathSegments: %d, path_length: %f, distToBlock: %f, followBlockNormalPath: %d\n",
        //      numPathSegments, path_length, distToBlock, followingBlockNormalPath_);

        
        // Skipping Dubins path since the straight line path seems to work fine as long as the steeringController gains
        // are set appropriately according to docking speed.
        //followingBlockNormalPath_ = true;
        
        // No reasonable Dubins path exists.
        // Either try again with smaller radii or just let the controller
        // attempt to get on to a straight line normal path.
        if (followingBlockNormalPath_) {
          
          // Compute new starting point for path
          // HACK: Feeling lazy, just multiplying path by some scalar so that it's likely to be behind the current robot pose.
          f32 x_start_mm = dockPose_.x() - 3 * distToBlock * cosf(dockPose_.angle.ToFloat());
          f32 y_start_mm = dockPose_.y() - 3 * distToBlock * sinf(dockPose_.angle.ToFloat());
          
          PathFollower::ClearPath();
          PathFollower::AppendPathSegment_Line(0, x_start_mm, y_start_mm, dockPose_.x(), dockPose_.y(),
                                               DOCK_APPROACH_SPEED_MMPS, DOCK_APPROACH_ACCEL_MMPS2, DOCK_APPROACH_DECEL_MMPS2);

          //PRINT("Computing straight line path (%f, %f) to (%f, %f)\n", x_start_m, y_start_m, dockPose_.x(), dockPose_.y());
        }

        /*
        // Set speed
        // TODO: Add hysteresis
        if (distToBlock < FAR_DIST_TO_BLOCK_THRESH_MM) {
          SpeedController::SetUserCommandedDesiredVehicleSpeed( DOCK_APPROACH_SPEED_MMPS );
        } else {
          SpeedController::SetUserCommandedDesiredVehicleSpeed( DOCK_FAR_APPROACH_SPEED_MMPS );
        }
        SpeedController::SetUserCommandedAcceleration( DOCK_APPROACH_ACCEL_MMPS2 );
        */
        
        // Start following path
        createdValidPath_ = PathFollower::StartPathTraversal(0, useManualSpeed_);
        
        // Debug
        if (!createdValidPath_) {
          PRINT("ERROR DockingController: Failed to create path\n");
          PathFollower::PrintPath();
        }
        
      }
      
      
      void StartDocking(const f32 dockOffsetDistX, const f32 dockOffsetDistY, const f32 dockOffsetAngle,
                        const bool useManualSpeed,
                        const u32 pointOfNoReturnDistMM)
      {
        dockOffsetDistX_ = dockOffsetDistX;
        
        useManualSpeed_ = useManualSpeed;
        pointOfNoReturnDistMM_ = pointOfNoReturnDistMM;
        pastPointOfNoReturn_ = false;
        lastDockingErrorSignalRecvdTime_ = HAL::GetTimeStamp();
        doHighDockLiftTracking_ = false;
        mode_ = LOOKING_FOR_BLOCK;
        markerlessDocking_ = false;
        success_ = false;
        
#if(DOCK_ANGLE_DAMPING)
        dockPoseAngleInitialized_ = false;
#endif
      }
      
      void StartDockingToRelPose(const f32 rel_x, const f32 rel_y, const f32 rel_angle, const bool useManualSpeed)
      {
        useManualSpeed_ = useManualSpeed;
        lastDockingErrorSignalRecvdTime_ = HAL::GetTimeStamp();
        mode_ = LOOKING_FOR_BLOCK;
        markerlessDocking_ = true;
        success_ = false;
        
        // NOTE: mode_ must be set to LOOKING_FOR_BLOCK and success_ must be false
        //       before we call SetRelDockPose()
        SetRelDockPose(rel_x, rel_y, rel_angle);
      }

      void ResetDocker() {
        

        SpeedController::SetUserCommandedDesiredVehicleSpeed(0);
        PathFollower::ClearPath();
        SteeringController::ExecuteDirectDrive(0,0);
        mode_ = IDLE;
        
        /* Don't reset these so we can query them after docking fails
         (e.g. to compute backout distance)
        lastMarkerDistX_ = 0.f;
        lastMarkerDistY_ = 0.f;
        lastMarkerAng_ = 0.f;
        */

        pointOfNoReturnDistMM_ = 0;
        pastPointOfNoReturn_ = false;
        markerlessDocking_ = false;
        markerOutOfFOV_ = false;
        lastMarkerPoseObservedIsSet_ = false;
        doHighDockLiftTracking_ = false;
        success_ = false;
      }
      
      bool GetLastMarkerPose(f32 &x, f32 &y, f32 &angle)
      {
        if (lastMarkerDistX_ > 0.f) {
          x = lastMarkerDistX_;
          y = lastMarkerDistY_;
          angle = lastMarkerAng_;
          return true;
        }
        return false;
      }
      
      void SetDockingErrorSignalMessage(const Messages::DockingErrorSignal& msg)
      {
        dockingErrSignalMsg_ = msg;
        dockingErrSignalMsgReady_ = true;
      }
      
      
      } // namespace DockingController
    } // namespace Cozmo
  } // namespace Anki
