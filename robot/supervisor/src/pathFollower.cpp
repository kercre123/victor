#include "dockingController.h"
#include "pathFollower.h"
#include "pickAndPlaceController.h"
#include "localization.h"
#include "steeringController.h"
#include "wheelController.h"
#include "speedController.h"
#include <math.h>
#include <float.h>
#include "trig_fast.h"
#include "velocityProfileGenerator.h"
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/hal.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "messages.h"


#define DEBUG_PATH_FOLLOWER 0

// At low speeds Cozmo was stuttering/struggling while following a path which was due to resetting
// wheelcontroller's integral gains, this enables or disables resetting those gains. If Cozmo suddenly
// has trouble following paths this can be reenabled
#define RESET_INTEGRAL_GAINS_AT_END_OF_SEGMENT 0

// The number of tics desired in between debug prints
#define DBG_PERIOD 200

// Whether or not path speeds should be automatically
// clamped to prevent driving off of cliffs
#define CLAMP_TO_CLIFF_SAFE_SPEEDS 0

namespace Anki
{
  namespace Vector
  {
    namespace PathFollower
    {

      namespace {

        const f32 CONTINUITY_TOL_MM2 = 1;

        const f32 LOOK_AHEAD_DIST_MM = 10;

        const f32 TOO_FAR_FROM_PATH_DIST_MM = 50;

        Planning::Path path_;
        s8 currPathSegment_ = -1;  // Segment index within local path array.
        s8 realPathSegment_ = -1;  // Segment index of the global path. Reset only on StartPathTraversal().

        // Shortest distance to path
        f32 distToPath_mm_ = 0;

        // Angular error with path
        f32 radToPath_ = 0;

        bool pointTurnStarted_ = false;

        // Whether or not the current path was specified internally or came from an
        // external source (via "execute path" message)
        bool currentPathIsExternal_ = false;

        // ID of the current path that is being followed
        // or the last path that was followed if not currently path following
        u16 lastPathID_ = 0;

        const f32 COAST_VELOCITY_MMPS = 25.f;
        const f32 COAST_VELOCITY_RADPS = 0.4f; // Same as POINT_TURN_TERMINAL_VEL_RAD_PER_S

        // Target speed to decelerate to when slowing down at end of segment
        const u16 END_OF_PATH_TARGET_SPEED_MMPS = 20;

        // Whether or not deceleration to end of current segment has started
        bool startedDecelOnSegment_ = false;

      } // Private Members


      Result Init(void)
      {
        ClearPath();
        lastPathID_ = 0;
        currentPathIsExternal_ = false;
        return RESULT_OK;
      }

      void SendPathFollowingEvent(PathEventType event)
      {
        RobotInterface::PathFollowingEvent pathEventMsg;
        pathEventMsg.pathID = lastPathID_;
        pathEventMsg.eventType = event;
        RobotInterface::SendMessage(pathEventMsg);
      }

      f32 GetCurrentSegmentTargetSpeed()
      {
        f32 speed = path_[currPathSegment_].GetTargetSpeed();
#if (CLAMP_TO_CLIFF_SAFE_SPEEDS==1)
        if (path_[currPathSegment_].GetType() != Planning::PST_POINT_TURN) {
          f32 maxSpeed = PickAndPlaceController::IsCarryingBlock() ? 
                         MAX_SAFE_WHILE_CARRYING_WHEEL_SPEED_MMPS : 
                         MAX_SAFE_WHEEL_SPEED_MMPS;
          speed = CLIP(speed, -maxSpeed, maxSpeed);
        }
#endif
        return speed;
      }

      // Deletes current path
      void ClearPath(bool didCompletePath)
      {
        if (IsTraversingPath())
        {
          SpeedController::SetBothDesiredAndCurrentUserSpeed(0);
          SpeedController::SetDefaultAccelerationAndDeceleration();

          if(!didCompletePath && currentPathIsExternal_)
          {
            // Send a path interruption event iff we were following an externally-specified path when cleared
            SendPathFollowingEvent(PathEventType::PATH_INTERRUPTED);
          }
        }

        path_.Clear();
        currPathSegment_ = -1;
        realPathSegment_ = -1;

        pointTurnStarted_ = false;

        distToPath_mm_ = 0.f;
        radToPath_ = 0.f;
        startedDecelOnSegment_ = false;

        currentPathIsExternal_ = false;

      } // Update()


      // Trims off segments that have already been traversed
      void TrimPath() {
        if (currPathSegment_ > 0) {
          TrimPath(currPathSegment_, 0);
        }
      }


      // Add path segment
      bool AppendPathSegment_Line(f32 x_start_mm, f32 y_start_mm, f32 x_end_mm, f32 y_end_mm,
                                  f32 targetSpeed, f32 accel, f32 decel)
      {
        TrimPath();
        return path_.AppendLine(x_start_mm, y_start_mm, x_end_mm, y_end_mm,
                                targetSpeed, accel, decel);
      }


      bool AppendPathSegment_Arc(f32 x_center_mm, f32 y_center_mm, f32 radius_mm, f32 startRad, f32 sweepRad,
                                 f32 targetSpeed, f32 accel, f32 decel)
      {
        TrimPath();
        return path_.AppendArc(x_center_mm, y_center_mm, radius_mm, startRad, sweepRad,
                               targetSpeed, accel, decel);
      }


      bool AppendPathSegment_PointTurn(f32 x, f32 y, f32 startAngle, f32 targetAngle,
                                       f32 targetRotSpeed, f32 rotAccel, f32 rotDecel,
                                       f32 angleTolerance,
                                       bool useShortestDir)
      {
        TrimPath();
        return path_.AppendPointTurn(x, y, startAngle, targetAngle,
                                     targetRotSpeed, rotAccel, rotDecel,
                                     angleTolerance,
                                     useShortestDir);
      }

      u8 GenerateDubinsPath(f32 start_x, f32 start_y, f32 start_theta,
                            f32 end_x, f32 end_y, f32 end_theta,
                            f32 start_radius, f32 end_radius,
                            f32 targetSpeed, f32 accel, f32 decel,
                            f32 final_straight_approach_length,
                            f32 *path_length)
      {
        return Planning::GenerateDubinsPath(path_,
                                            start_x, start_y, start_theta,
                                            end_x, end_y, end_theta,
                                            start_radius, end_radius,
                                            targetSpeed, accel, decel,
                                            final_straight_approach_length,
                                            path_length);
      }

      u8 GetClosestSegment(const f32 x, const f32 y, const f32 angle)
      {
        AnkiAssert(path_.GetNumSegments() > 0, "");

        Planning::SegmentRangeStatus res;
        f32 distToSegment, angError;

        u8 closestSegId = 0;
        f32 distToClosestSegment = FLT_MAX;

        for (u8 i=0; i<path_.GetNumSegments(); ++i) {
          res = path_[i].GetDistToSegment(x,y,angle,distToSegment,angError);
#if(DEBUG_PATH_FOLLOWER)
          AnkiDebug( "PathFollower.GetClosestSegment.PathDist", "%f  (res=%d)", distToSegment, res);
#endif
          if (ABS(distToSegment) < distToClosestSegment && (res == Planning::IN_SEGMENT_RANGE || res == Planning::OOR_NEAR_START)) {
            closestSegId = i;
            distToClosestSegment = ABS(distToSegment);
#if(DEBUG_PATH_FOLLOWER)
            AnkiDebug( "PathFollower.GetClosestSegment", " New closest seg: %d, distToSegment %f (res=%d)", i, distToSegment, res);
#endif
          }
        }

        return closestSegId;
      }


      void TrimPath(const u8 numPopFrontSegments, const u8 numPopBackSegments)
      {
        path_.PopBack(numPopBackSegments);
        if (path_.PopFront(numPopFrontSegments) && currPathSegment_ > 0) {
          currPathSegment_ -= numPopFrontSegments;
        }
      }

      bool StartPathTraversal(u16 path_id)
      {
        // If we were already following an externally-specified path, send an interruption event.
        if(IsTraversingPath() && currentPathIsExternal_)
        {
          SendPathFollowingEvent(PathEventType::PATH_INTERRUPTED);
        }

        // Set first path segment
        if (path_.GetNumSegments() > 0) {

#if(DEBUG_PATH_FOLLOWER)
          path_.PrintPath();
#endif

          AnkiConditionalErrorAndReturnValue(path_.CheckContinuity(CONTINUITY_TOL_MM2), false, "PathFollower.StartPathTraversal.PathIsDiscontinuous", "");

          currPathSegment_ = 0;
          realPathSegment_ = currPathSegment_;
          startedDecelOnSegment_ = false;


          // Set speed
          // (Except for point turns whose speeds are handled at the steering controller level)
          f32 targetSpeed = GetCurrentSegmentTargetSpeed();
          if (path_[currPathSegment_].GetType() != Planning::PST_POINT_TURN) {
            SpeedController::SetUserCommandedDesiredVehicleSpeed( targetSpeed );
            SpeedController::SetUserCommandedAcceleration( path_[currPathSegment_].GetAccel() );
            SpeedController::SetUserCommandedDeceleration( path_[currPathSegment_].GetDecel() );
          }

          AnkiDebug( "PathFollower.StartPathTraversal", "Start segment %d, speed = %f, accel = %f, decel = %f",
                currPathSegment_,
                targetSpeed,
                path_[currPathSegment_].GetAccel(),
                path_[currPathSegment_].GetDecel());

          SteeringController::SetPathFollowMode();

        }

        // Is newly-specified path from an external source?
        currentPathIsExternal_ = (path_id != 0);

        if(currentPathIsExternal_)
        {
          // update the lastPathID and send an event that we're starting a new externally-specified path
          lastPathID_ = path_id;
          SendPathFollowingEvent(PathEventType::PATH_STARTED);
        }

        return TRUE;
      }

      bool IsTraversingPath()
      {
        return currPathSegment_ >= 0;
      }

      s8 GetCurrPathSegment()
      {
        return realPathSegment_;// currPathSegment_;
      }

      Planning::SegmentRangeStatus ProcessPathSegment(f32 &shortestDistanceToPath_mm, f32 &radDiff)
      {
        // Get current robot pose
        f32 distToEnd = 0;
        f32 x, y, lookaheadX, lookaheadY;
        Radians angle;
        Localization::GetDriveCenterPose(x, y, angle);

        lookaheadX = x;
        lookaheadY = y;

        AnkiAssert(Planning::PST_LINE == path_[currPathSegment_].GetType() || Planning::PST_ARC == path_[currPathSegment_].GetType(), "");

        // Compute lookahead position
        if (LOOK_AHEAD_DIST_MM != 0) {
          if (path_[currPathSegment_].GetTargetSpeed() > 0) {
            lookaheadX += LOOK_AHEAD_DIST_MM * cosf(angle.ToFloat());
            lookaheadY += LOOK_AHEAD_DIST_MM * sinf(angle.ToFloat());
          } else {
            lookaheadX -= LOOK_AHEAD_DIST_MM * cosf(angle.ToFloat());
            lookaheadY -= LOOK_AHEAD_DIST_MM * sinf(angle.ToFloat());
          }
        }

        Planning::SegmentRangeStatus status = path_[currPathSegment_].GetDistToSegment(lookaheadX,lookaheadY,angle.ToFloat(),shortestDistanceToPath_mm,radDiff, &distToEnd);

        // If this is the last segment or the next segment is a point turn we need to
        // (1) check if the lookahead point is out of range and if so, use the
        //     robot drive center to compute distance to segment, and
        // (2) decelerate towards the end of the piece according to the segment's
        //     deceleration or faster if necessary.
        if ((currPathSegment_ == path_.GetNumSegments() - 1) || (path_[currPathSegment_+1].GetType() == Planning::PST_POINT_TURN)) {

          // 1) Check if time to switch to robot drive center instead of origin
          if (status == Planning::OOR_NEAR_END) {
            if (LOOK_AHEAD_DIST_MM != 0) {
              f32 junk_mm, junk_rad;
              status = path_[currPathSegment_].GetDistToSegment(x,y,angle.ToFloat(),junk_mm, junk_rad, &distToEnd);
              //PRINT("PATH-OOR: status %d (distToEnd %f, currCmdSpeed %d mm/s, currSpeed %d mm/s)\n", status, distToEnd, (s32)
              //      SpeedController::GetUserCommandedCurrentVehicleSpeed(), (s32)SpeedController::GetCurrentMeasuredVehicleSpeed());
            }
          } else {
            // For purposes of determining if we should decelerate we should consider the
            // distance remaining between the segment end and the robot's drive center
            // rather than the lookahead point.
            distToEnd += LOOK_AHEAD_DIST_MM;
          }

          // 2) Check if time to decelerate
          if (!startedDecelOnSegment_) {
            // See if the current deceleration rate is sufficient to stop the robot at the end of the path
            u16 decel = SpeedController::GetUserCommandedDeceleration();
            s32 currSpeed = SpeedController::GetUserCommandedCurrentVehicleSpeed();
            f32 distToStopIfDecelNow = 0.5f*currSpeed*currSpeed / decel;

            // Is it time to start slowing down?
            if (distToStopIfDecelNow >= distToEnd) {
              // Compute the deceleration necessary to stop robot at end of segment in the remaining distance
              // Since we're actually decelerating to END_OF_PATH_TARGET_SPEED_MMPS, this is just an approximation.
              u16 requiredDecel = 0.5f*currSpeed*currSpeed / distToEnd;
              SpeedController::SetUserCommandedDeceleration(requiredDecel);
              SpeedController::SetUserCommandedDesiredVehicleSpeed(copysign(END_OF_PATH_TARGET_SPEED_MMPS, path_[currPathSegment_].GetTargetSpeed()));
              startedDecelOnSegment_ = true;
              //AnkiDebug( "PathFollower", "PathFollower: Decel to end of segment %d (of %d) at %d mm/s^2 from speed of %d mm/s (meas %d mm/s) over %f mm\n",
              //      currPathSegment_, path_.GetNumSegments(), requiredDecel, currSpeed, (s32)SpeedController::GetCurrentMeasuredVehicleSpeed(), distToEnd);
            }
          }
#         if(DEBUG_PATH_FOLLOWER)
          else {
            AnkiDebug( "PathFollower.ProcessPathSegment.Decel", "currCmdSpeed %d mm/s, currSpeed %d mm/s)",
                  (s32)SpeedController::GetUserCommandedCurrentVehicleSpeed(),
                  (s32)SpeedController::GetCurrentMeasuredVehicleSpeed());
          }
#         endif
        }

        return status;
      }



      Planning::SegmentRangeStatus ProcessPathSegmentPointTurn(f32 &shortestDistanceToPath_mm, f32 &radDiff)
      {
        const Planning::PathSegmentDef::s_turn* currSeg = &(path_[currPathSegment_].GetDef().turn);

#if(DEBUG_PATH_FOLLOWER)
        Radians currOrientation = Localization::GetCurrPose_angle();
        AnkiDebug( "PathFollower.ProcessPathSegmentPointTurn", "currPathSeg: %d, TURN currAngle: %f, targetAngle: %f",
               currPathSegment_, currOrientation.ToFloat(), currSeg->targetAngle);
#endif

        // When the car is stopped, initiate point turn
        if (!pointTurnStarted_) {
#if(DEBUG_PATH_FOLLOWER)
          AnkiDebug( "PathFollower.ProcessPathSegmentPointTurn.ExecutePointTurn", "");
#endif
          SteeringController::ExecutePointTurn(currSeg->targetAngle,
                                               path_[currPathSegment_].GetTargetSpeed(),
                                               path_[currPathSegment_].GetAccel(),
                                               path_[currPathSegment_].GetDecel(),
                                               currSeg->angleTolerance,
                                               currSeg->useShortestDir);
          pointTurnStarted_ = true;

        } else {
          if (SteeringController::GetMode() != SteeringController::SM_POINT_TURN) {
            pointTurnStarted_ = false;
            return Planning::OOR_NEAR_END;
          }
        }

        // For point turns, ignore how far off we may be from the ideal point turn location
        shortestDistanceToPath_mm = 0.f;

        return Planning::IN_SEGMENT_RANGE;
      }

      // Post-path completion cleanup
      void PathComplete()
      {
        // Send a path completion event iff this is an externally-specified path
        if(currentPathIsExternal_)
        {
          SendPathFollowingEvent(PathEventType::PATH_COMPLETED);
        }

        // Pass in "true" to signify that we _did_ complete the path (so don't send an Interruption event, since
        // we just sent a Completed event above).
        ClearPath(true);

        AnkiInfo("PathFollower.PathComplete", "");
      }


      bool GetPathError(f32 &shortestDistanceToPath_mm, f32 &radDiff)
      {
        if (!IsTraversingPath()) {
          return false;
        }

        shortestDistanceToPath_mm = distToPath_mm_;
        radDiff = radToPath_;
        return true;
      }



      Result Update()
      {
        if (currPathSegment_ < 0) {
          return RESULT_OK;
        }

        Planning::SegmentRangeStatus segRes = Planning::OOR_NEAR_END;
        switch (path_[currPathSegment_].GetType()) {
          case Planning::PST_LINE:
          case Planning::PST_ARC:
            segRes = ProcessPathSegment(distToPath_mm_, radToPath_);
            break;
          case Planning::PST_POINT_TURN:
            segRes = ProcessPathSegmentPointTurn(distToPath_mm_, radToPath_);
            break;
          default:
            AnkiWarn( "PathFollower.Update.InvalidSegmentType", "Segment %d has invalid type %d", currPathSegment_, path_[currPathSegment_].GetType());
            break;
        }

#if(DEBUG_PATH_FOLLOWER)
        AnkiDebug( "PathFollower.Update.DistToPath", "%f mm, %f deg, segRes %d, segType %d, currSeg %d", distToPath_mm_, RAD_TO_DEG_F32(radToPath_), segRes, path_[currPathSegment_].GetType(), currPathSegment_);
#endif

        // Go to next path segment if no longer in range of the current one
        if (segRes == Planning::OOR_NEAR_END) {
          if (++currPathSegment_ >= path_.GetNumSegments()) {
            // Path is complete
            PathComplete();
            return RESULT_OK;
          }
          ++realPathSegment_;
          startedDecelOnSegment_ = false;

          // Command new speed for segment
          // (Except for point turns whose speeds are handled at the steering controller level)
          f32 targetSpeed = GetCurrentSegmentTargetSpeed();
          if (path_[currPathSegment_].GetType() != Planning::PST_POINT_TURN) {
            SpeedController::SetUserCommandedDesiredVehicleSpeed( targetSpeed );
            SpeedController::SetUserCommandedAcceleration( path_[currPathSegment_].GetAccel() );
            SpeedController::SetUserCommandedDeceleration( path_[currPathSegment_].GetDecel() );
          }
#if(DEBUG_PATH_FOLLOWER)
          AnkiDebug( "PathFollower.Update.SegmentSpeed", "Segment %d, speed = %f, accel = %f, decel = %f",
                currPathSegment_,
                targetSpeed,
                path_[currPathSegment_].GetAccel(),
                path_[currPathSegment_].GetDecel());
#endif

#if(RESET_INTEGRAL_GAINS_AT_END_OF_SEGMENT)
          WheelController::ResetIntegralGainSums();
#endif

        }

        if (!DockingController::IsBusy()) {
          // Check that starting error is not too big
          // TODO: Check for excessive heading error as well?
          if (ABS(distToPath_mm_) > TOO_FAR_FROM_PATH_DIST_MM) {
            AnkiWarn( "PathFollower.Update.StartingErrorTooLarge", "%f mm", distToPath_mm_);
            ClearPath();
            return RESULT_FAIL;
          }
        }

        return RESULT_OK;
      }


      void PrintPath()
      {
        path_.PrintPath();
      }

      void PrintPathSegment(s16 segment)
      {
        path_.PrintSegment(segment);
      }

      u16 GetLastPathID() {
        return lastPathID_;
      }

      const Planning::Path& GetPath()
      {
        return path_;
      }


      bool DriveStraight(f32 dist_mm, f32 acc_start_frac, f32 acc_end_frac, f32 duration_sec)
      {
        VelocityProfileGenerator vpg;

        // Compute profile
        f32 curr_x, curr_y;
        Radians curr_angle;
        Localization::GetDriveCenterPose(curr_x, curr_y, curr_angle);
        f32 currSpeed = SpeedController::GetCurrentMeasuredVehicleSpeed();

        // Check for valid fractions
        if (acc_start_frac < 0 || acc_end_frac < 0) {
          AnkiWarn( "PathFollower.DriveStraight.NegativeFraction", "start: %f, end: %f", acc_start_frac, acc_end_frac);
          return false;
        }

        acc_start_frac = MAX(acc_start_frac, 0.01f);
        acc_end_frac   = MAX(acc_end_frac,   0.01f);

        if (!vpg.StartProfile_fixedDuration(0, currSpeed, acc_start_frac * duration_sec,
                                            dist_mm, acc_end_frac * duration_sec,
                                            MAX_SAFE_WHEEL_SPEED_MMPS, MAX_WHEEL_ACCEL_MMPS2,
                                            duration_sec, CONTROL_DT) ) {
          AnkiWarn( "PathFollower.DriveStraight.VPGFail", "");
          return false;
        }


        // Compute the destination pose
        f32 dest_x = curr_x + dist_mm * cosf(curr_angle.ToFloat());
        f32 dest_y = curr_y + dist_mm * sinf(curr_angle.ToFloat());


        // Compute start and end acceleration distances.
        // Some compensation here for lookahead distance.
        // NOTE: PathFollower actually transitions to the speed settings of the following segment
        //       at LOOK_AHEAD_DIST_MM before it actually reaches the next segment.
        f32 startAccelDist = vpg.GetStartAccelDist();
        f32 endAccelDist = vpg.GetEndAccelDist();
        if (fabsf(endAccelDist) > LOOK_AHEAD_DIST_MM) {
          endAccelDist += LOOK_AHEAD_DIST_MM * signbit(endAccelDist);
        }
        AnkiInfo( "PathFollower.DriveStraight.Params", "total dist %f, startDist %f, endDist %f", dist_mm, startAccelDist, endAccelDist);

        // Get intermediate poses: (1) after starting accel phase and (2) before ending accel phase
        f32 int_x1 = curr_x + startAccelDist * cosf(curr_angle.ToFloat());
        f32 int_y1 = curr_y + startAccelDist * sinf(curr_angle.ToFloat());

        f32 int_x2 = dest_x - endAccelDist * cosf(curr_angle.ToFloat());
        f32 int_y2 = dest_y - endAccelDist * sinf(curr_angle.ToFloat());


        // Get intermediate speed and accels
        f32 maxReachableVel = vpg.GetMaxReachableVel();
        f32 startAccel = fabsf(vpg.GetStartAccel());
        f32 endAccel = fabsf(vpg.GetEndAccel());


        // Create 3-segment path
        AnkiDebug( "PathFollower.DriveStraight.Accels", "start %f, end %f, vel %f\n", startAccel, endAccel, maxReachableVel);
        AnkiDebug( "PathFollower.DriveStraight.Points", "(%f, %f) to (%f, %f) to (%f, %f) to (%f, %f)\n", curr_x, curr_y, int_x1, int_y1, int_x2, int_y2, dest_x, dest_y);
        ClearPath();
        AppendPathSegment_Line(curr_x, curr_y, int_x1, int_y1, maxReachableVel, startAccel, startAccel);
        AppendPathSegment_Line(int_x1, int_y1, int_x2, int_y2, maxReachableVel, startAccel, startAccel);
        AppendPathSegment_Line(int_x2, int_y2, dest_x, dest_y, dist_mm > 0 ? COAST_VELOCITY_MMPS : -COAST_VELOCITY_MMPS, endAccel, endAccel);
        StartPathTraversal();

        return true;
      }

      bool DriveArc(f32 sweep_rad, f32 radius_mm, f32 acc_start_frac, f32 acc_end_frac, f32 duration_sec)
      {
        VelocityProfileGenerator vpg;

        // Compute profile
        f32 curr_x, curr_y;
        Radians curr_angle;
        Localization::GetDriveCenterPose(curr_x, curr_y, curr_angle);
        f32 currAngSpeed = -SpeedController::GetCurrentMeasuredVehicleSpeed() / radius_mm;

        // Check for valid fractions
        if (acc_start_frac < 0 || acc_end_frac < 0) {
          AnkiWarn( "PathFollower.DriveArc.NegativeFraction", "start: %f, end: %f", acc_start_frac, acc_end_frac);
          return false;
        }

        acc_start_frac = MAX(acc_start_frac, 0.01f);
        acc_end_frac   = MAX(acc_end_frac,   0.01f);

        if (!vpg.StartProfile_fixedDuration(0, currAngSpeed, acc_start_frac * duration_sec,
                                            sweep_rad, acc_end_frac * duration_sec,
                                            MAX_BODY_ROTATION_SPEED_RAD_PER_SEC, MAX_BODY_ROTATION_ACCEL_RAD_PER_SEC2,
                                            duration_sec, CONTROL_DT) ) {
          AnkiWarn( "PathFollower.DriveArc.VPGFail", "");
          return false;
        }

        // Compute x_center,y_center
        f32 angToCenter = curr_angle.ToFloat() + (radius_mm > 0 ? -1 : 1) * M_PI_2_F;
        f32 absRadius = fabsf(radius_mm);
        f32 x_center = curr_x + absRadius * cosf(angToCenter);
        f32 y_center = curr_y + absRadius * sinf(angToCenter);

        // Compute startRad relative to (x_center, y_center)
        f32 startRad = angToCenter + M_PI_F;

        // Get intermediate poses: (1) after starting accel phase and (2) before ending accel phase
        f32 startAccelSweep = vpg.GetStartAccelDist();
        f32 endAccelSweep = vpg.GetEndAccelDist();
        //if (endAccelDist < -LOOK_AHEAD_DIST_MM) {
        //  endAccelDist += LOOK_AHEAD_DIST_MM;
        //}


        f32 int_ang1 = startRad + startAccelSweep;
        f32 int_ang2 = startRad + sweep_rad - endAccelSweep;

        // Get intermediate speed and accels.
        // Need to convert angular speeds/accel into linear speeds/accel
        f32 targetAngSpeed = fabsf(vpg.GetMaxReachableVel());
        f32 startAngAccel = fabsf(vpg.GetStartAccel());
        f32 endAngAccel = fabsf(vpg.GetEndAccel());


        u8 drivingFwd = (signbit(sweep_rad) != signbit(radius_mm)) ? 1 : -1;
        f32 targetSpeed = drivingFwd * targetAngSpeed * absRadius;
        f32 startAccel = startAngAccel * absRadius;
        f32 endAccel = endAngAccel * absRadius;


        AnkiDebug( "PathFollower.DriveArc", "curr_x,y  (%f, %f), center x,y (%f, %f), radius %f", curr_x, curr_y, x_center, y_center, radius_mm);
        AnkiDebug( "PathFollower.DriveArc", "start + sweep1 = ang1 (%f + %f = %f), end + sweep2 = ang2 ang2 (%f - %f = %f)", startRad, startAccelSweep, int_ang1, startRad + sweep_rad, endAccelSweep, int_ang2);
        AnkiDebug( "PathFollower.DriveArc", "targetSpeed %f, startAccel %f, endAccel %f", targetSpeed, startAccel, endAccel);

        // Create 3-segment path
        ClearPath();
        AppendPathSegment_Arc(x_center, y_center, absRadius, startRad, startAccelSweep, targetSpeed, startAccel, startAccel);
        AppendPathSegment_Arc(x_center, y_center, absRadius, int_ang1, int_ang2-int_ang1, targetSpeed, startAccel, startAccel);
        AppendPathSegment_Arc(x_center, y_center, absRadius, int_ang2, endAccelSweep, drivingFwd > 0 ? COAST_VELOCITY_MMPS : -COAST_VELOCITY_MMPS, endAccel, endAccel);

        StartPathTraversal();

        return true;
      }

      bool DrivePointTurn(f32 sweep_rad, f32 acc_start_frac, f32 acc_end_frac, f32 angleTolerance, f32 duration_sec)
      {
        VelocityProfileGenerator vpg;

        f32 curr_x, curr_y;
        Radians curr_angle;
        Localization::GetDriveCenterPose(curr_x, curr_y, curr_angle);

        // Check for valid fractions
        if (acc_start_frac < 0 || acc_end_frac < 0) {
          AnkiWarn( "PathFollower.DrivePointTurn.NegativeFraction", "start: %f, end: %f", acc_start_frac, acc_end_frac);
          return false;
        }

        acc_start_frac = MAX(acc_start_frac, 0.01f);
        acc_end_frac   = MAX(acc_end_frac,   0.01f);

        if (!vpg.StartProfile_fixedDuration(0, 0, acc_start_frac * duration_sec,
                                            sweep_rad, acc_end_frac * duration_sec,
                                            MAX_BODY_ROTATION_SPEED_RAD_PER_SEC, MAX_BODY_ROTATION_ACCEL_RAD_PER_SEC2,
                                            duration_sec, CONTROL_DT)) {
          AnkiWarn( "PathFollower.DrivePointTurn.VPGFail", "sweep_rad: %f, acc_start_frac %f, acc_end_frac %f, duration_sec %f",
                   sweep_rad, acc_start_frac, acc_end_frac, duration_sec);
          return false;
        }


        f32 targetRotVel = vpg.GetMaxReachableVel();
        f32 startAccelSweep = vpg.GetStartAccelDist();
        f32 endAccelSweep = vpg.GetEndAccelDist();
        f32 startAngAccel = fabsf(vpg.GetStartAccel());
        f32 endAngAccel = fabsf(vpg.GetEndAccel());

        // Compute intermediate angles
        f32 dest_ang = curr_angle.ToFloat() + sweep_rad;
        f32 int_ang1 = curr_angle.ToFloat() + startAccelSweep;
        f32 int_ang2 = dest_ang - endAccelSweep;


        AnkiDebug( "PathFollower.DrivePointTurn", "start %f, int_ang1 %f, int_ang2 %f, dest %f", curr_angle.ToFloat(), int_ang1, int_ang2, dest_ang);
        AnkiDebug( "PathFollower.DrivePointTurn", "targetRotSpeed %f, startRotAccel %f, endRotAccel %f", targetRotVel, startAngAccel, endAngAccel);

        // Create 3-segment path
        ClearPath();
        AppendPathSegment_PointTurn(curr_x, curr_y, curr_angle.ToFloat(), int_ang1, targetRotVel, startAngAccel, startAngAccel, angleTolerance, false);
        AppendPathSegment_PointTurn(curr_x, curr_y, int_ang1, int_ang2, targetRotVel, startAngAccel, startAngAccel, angleTolerance, false);
        AppendPathSegment_PointTurn(curr_x, curr_y, int_ang2, dest_ang, sweep_rad > 0 ? COAST_VELOCITY_RADPS : -COAST_VELOCITY_RADPS, endAngAccel, endAngAccel, angleTolerance, false);

        StartPathTraversal();

        return true;
      }





    } // namespace PathFollower
  } // namespace Vector
} // namespace Anki
