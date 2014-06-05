#include "anki/cozmo/robot/debug.h"
#include "dockingController.h"
#include "anki/planning/shared/path.h"
#include "pathFollower.h"
#include "localization.h"
#include "steeringController.h"
#include "wheelController.h"
#include "speedController.h"

#include "anki/common/robot/utilities_c.h"
#include "anki/common/robot/trig_fast.h"

#include "anki/cozmo/robot/hal.h"


#define DEBUG_PATH_FOLLOWER 0


#ifndef SIMULATOR
#define ENABLE_PATH_VIZ 0  // This must always be 0!
#else
#include "sim_viz.h"
using namespace Anki::Cozmo::Sim;
#define ENABLE_PATH_VIZ 0  // To enable visualization of paths from robot
                           // (Default is 0. Normally this is done from basestation.)
#endif

// The number of tics desired in between debug prints
#define DBG_PERIOD 200

namespace Anki
{
  namespace Cozmo
  {
    namespace PathFollower
    {
      
      namespace {
        
        const f32 CONTINUITY_TOL_MM2 = 1;
        
        const f32 LOOK_AHEAD_DIST_MM = 10;
        
        const f32 TOO_FAR_FROM_PATH_DIST_MM = 50;

        Planning::Path path_;
        s8 currPathSegment_ = -1;
        
        // Shortest distance to path
        f32 distToPath_mm_ = 0;
        
        // Angular error with path
        f32 radToPath_ = 0;
        
        bool pointTurnStarted_ = false;
        
      } // Private Members
      
      
      Result Init(void)
      {
        ClearPath();
        
        return RESULT_OK;
      }
      
      
      // Deletes current path
      void ClearPath(void)
      {
        path_.Clear();
        currPathSegment_ = -1;
#if(ENABLE_PATH_VIZ)
        Viz::ErasePath(0);
#endif
      } // Update()
      
      
      void SetPathForViz() {
#if(ENABLE_PATH_VIZ)
        Viz::ErasePath(0);
        for (u8 i=0; i<path_.GetNumSegments(); ++i) {
          const Planning::PathSegmentDef& ps = path_[i].GetDef();
          switch(path_[i].GetType()) {
            case Planning::PST_LINE:
              Viz::AppendPathSegmentLine(0,
                                         ps.line.startPt_x,
                                         ps.line.startPt_y,
                                         ps.line.endPt_x,
                                         ps.line.endPt_y);
              break;
            case Planning::PST_ARC:
              Viz::AppendPathSegmentArc(0,
                                        ps.arc.centerPt_x,
                                        ps.arc.centerPt_y,
                                        ps.arc.radius,
                                        ps.arc.startRad,
                                        ps.arc.sweepRad);
              break;
            default:
              break;
          }
        }
#endif
      }
      
      
      
      // Add path segment
      // TODO: Change units to meters
      bool AppendPathSegment_Line(u32 matID, f32 x_start_mm, f32 y_start_mm, f32 x_end_mm, f32 y_end_mm,
                                  f32 targetSpeed, f32 accel, f32 decel)
      {
        return path_.AppendLine(matID, x_start_mm, y_start_mm, x_end_mm, y_end_mm,
                                targetSpeed, accel, decel);
      }
      
      
      bool AppendPathSegment_Arc(u32 matID, f32 x_center_mm, f32 y_center_mm, f32 radius_mm, f32 startRad, f32 sweepRad,
                                 f32 targetSpeed, f32 accel, f32 decel)
      {
        return path_.AppendArc(matID, x_center_mm, y_center_mm, radius_mm, startRad, sweepRad,
                               targetSpeed, accel, decel);
      }
      
      
      bool AppendPathSegment_PointTurn(u32 matID, f32 x, f32 y, f32 targetAngle,
                                       f32 targetRotSpeed, f32 rotAccel, f32 rotDecel)
      {
        return path_.AppendPointTurn(matID, x, y, targetAngle,
                                     targetRotSpeed, rotAccel, rotDecel);
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
        assert(path_.GetNumSegments() > 0);
        
        Planning::SegmentRangeStatus res;
        f32 distToSegment, angError;
        
        u8 closestSegId = 0;
        f32 distToClosestSegment = FLT_MAX;
        
        for (u8 i=0; i<path_.GetNumSegments(); ++i) {
          res = path_[i].GetDistToSegment(x,y,angle,distToSegment,angError);
#if(DEBUG_PATH_FOLLOWER)
          PRINT("PathDist: %f  (res=%d)\n", distToSegment, res);
#endif
          if (ABS(distToSegment) < distToClosestSegment && (res == Planning::IN_SEGMENT_RANGE || res == Planning::OOR_NEAR_START)) {
            closestSegId = i;
            distToClosestSegment = ABS(distToSegment);
#if(DEBUG_PATH_FOLLOWER)
            PRINT(" New closest seg: %d, distToSegment %f (res=%d)\n", i, distToSegment, res);
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
      
      bool StartPathTraversal()
      {
        // Set first path segment
        if (path_.GetNumSegments() > 0) {
          
#if(DEBUG_PATH_FOLLOWER)
          path_.PrintPath();
#endif
          
          if (!path_.CheckContinuity(CONTINUITY_TOL_MM2)) {
            PRINT("ERROR: Path is discontinuous\n");
            return false;
          }
          
          
          // Get current robot pose
          f32 x, y;
          Radians angle;
          Localization::GetCurrentMatPose(x, y, angle);
          
          //currPathSegment_ = 0;
          currPathSegment_ = GetClosestSegment(x,y,angle.ToFloat());

          // Set speed
          // (Except for point turns whose speeds are handled at the steering controller level)
          if (path_[currPathSegment_].GetType() != Planning::PST_POINT_TURN) {
            SpeedController::SetUserCommandedDesiredVehicleSpeed( path_[currPathSegment_].GetTargetSpeed() );
            SpeedController::SetUserCommandedAcceleration( path_[currPathSegment_].GetAccel() );
            SpeedController::SetUserCommandedDeceleration( path_[currPathSegment_].GetDecel() );
          }

#if(DEBUG_PATH_FOLLOWER)
          PRINT("*** PATH START SEGMENT %d: speed = %f, accel = %f, decel = %f\n",
                currPathSegment_,
                path_[currPathSegment_].GetTargetSpeed(),
                path_[currPathSegment_].GetAccel(),
                path_[currPathSegment_].GetDecel());
#endif

          //Robot::SetOperationMode(Robot::FOLLOW_PATH);
          SteeringController::SetPathFollowMode();
        }
        
        // Visualize path
        SetPathForViz();
        
        return TRUE;
      }
      
      
      bool IsTraversingPath()
      {
        return currPathSegment_ >= 0;
      }
      
      s8 GetCurrPathSegment()
      {
        return currPathSegment_;
      }
      
      Planning::SegmentRangeStatus ProcessPathSegment(f32 &shortestDistanceToPath_mm, f32 &radDiff)
      {
        // Get current robot pose
        f32 x, y, lookaheadX, lookaheadY;
        Radians angle;
        Localization::GetCurrentMatPose(x, y, angle);
        
        lookaheadX = x;
        lookaheadY = y;

        bool checkRobotOriginStatus = false;
        Planning::PathSegmentType currType = path_[currPathSegment_].GetType();
        
        // Compute lookahead position
        if (LOOK_AHEAD_DIST_MM != 0 && (currType == Planning::PST_LINE || currType == Planning::PST_ARC) ) {
          lookaheadX += LOOK_AHEAD_DIST_MM * cosf(angle.ToFloat());
          lookaheadY += LOOK_AHEAD_DIST_MM * sinf(angle.ToFloat());
          checkRobotOriginStatus = true;
        }
        
        Planning::SegmentRangeStatus status = path_[currPathSegment_].GetDistToSegment(lookaheadX,lookaheadY,angle.ToFloat(),shortestDistanceToPath_mm,radDiff);
        
        // If this is the last piece or the next piece is a point turn
        // check if the robot origin is out of range.
        if (status == Planning::OOR_NEAR_END &&
            checkRobotOriginStatus &&
            ((currPathSegment_ == path_.GetNumSegments() - 1)
             || (path_[currPathSegment_+1].GetType() == Planning::PST_POINT_TURN))
            ) {
          
          f32 junk_mm, junk_rad;
          status = path_[currPathSegment_].GetDistToSegment(x,y,angle.ToFloat(),junk_mm, junk_rad);
        }
        
        return status;
      }
      
      

      Planning::SegmentRangeStatus ProcessPathSegmentPointTurn(f32 &shortestDistanceToPath_mm, f32 &radDiff)
      {
        const Planning::PathSegmentDef::s_turn* currSeg = &(path_[currPathSegment_].GetDef().turn);
        
#if(DEBUG_PATH_FOLLOWER)
        Radians currOrientation = Localization::GetCurrentMatOrientation();
        PRINT("currPathSeg: %d, TURN  currAngle: %f, targetAngle: %f\n",
               currPathSegment_, currOrientation.ToFloat(), currSeg->targetAngle);
#endif
        
        // When the car is stopped, initiate point turn
        if (!pointTurnStarted_) {
#if(DEBUG_PATH_FOLLOWER)
          PRINT("EXECUTE POINT TURN\n");
#endif
          SteeringController::ExecutePointTurn(currSeg->targetAngle,
                                               path_[currPathSegment_].GetTargetSpeed(),
                                               path_[currPathSegment_].GetAccel(),
                                               path_[currPathSegment_].GetDecel());
          pointTurnStarted_ = true;

        } else {
          if (SteeringController::GetMode() != SteeringController::SM_POINT_TURN) {
            pointTurnStarted_ = false;
            return Planning::OOR_NEAR_END;
          }
        }

        
        return Planning::IN_SEGMENT_RANGE;
      }
      
      // Post-path completion cleanup
      void PathComplete()
      {
        pointTurnStarted_ = false;
        currPathSegment_ = -1;
#if(DEBUG_PATH_FOLLOWER)
        PRINT("*** PATH COMPLETE ***\n");
#endif
#if(ENABLE_PATH_VIZ)
        Viz::ErasePath(0);
#endif
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
        
#if(FREE_DRIVE_DUBINS_TEST)
        // Test code to visualize Dubins path online
        f32 start_x,start_y;
        Radians start_theta;
        Localization::GetCurrentMatPose(start_x,start_y,start_theta);
        path_.Clear();
        
        const f32 end_x = 0;
        const f32 end_y = 250;
        const f32 end_theta = 0.5*PI;
        const f32 start_radius = 50;
        const f32 end_radius = 50;
        const f32 targetSpeed = 100;
        const f32 accel = 200;
        const f32 decel = 200;
        const f32 final_straight_approach_length = 0.1;
        f32 path_length;
        u8 numSegments = Planning::GenerateDubinsPath(path_,
                                                      start_x, start_y, start_theta.ToFloat(),
                                                      end_x, end_y, end_theta,
                                                      start_radius, end_radius,
                                                      targetSpeed, accel, decel,
                                                      final_straight_approach_length,
                                                      &path_length);
        const f32 distToTarget = sqrtf((start_x - end_x)*(start_x - end_x) + (start_y - end_y)*(start_y - end_y));
        PERIODIC_PRINT(500, "Dubins Test: pathLength %f, distToTarget %f\n", path_length, distToTarget);
        
        // Test threshold for whether to use Dubins path or not)
        
        if (path_length > 2 * distToTarget) {
          PRINT(" Dubins path exceeds 2x dist to target (%f %f)\n", path_length, distToTarget);
        }
        
        //path_.PrintPath();
#if(ENABLE_PATH_VIZ)
        SetPathForViz();
        Viz::ShowPath(0, true);
#endif
#endif
        
        
        if (currPathSegment_ < 0) {
          SpeedController::SetUserCommandedDesiredVehicleSpeed(0);
          return RESULT_FAIL;
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
            // TODO: Error?
            break;
        }
        
#if(DEBUG_PATH_FOLLOWER)
        PERIODIC_PRINT(DBG_PERIOD,"PATH ERROR: %f mm, %f deg\n", distToPath_mm_, RAD_TO_DEG(radToPath_));
#endif
        
        // Go to next path segment if no longer in range of the current one
        if (segRes == Planning::OOR_NEAR_END) {
          if (++currPathSegment_ >= path_.GetNumSegments()) {
            // Path is complete
            PathComplete();
            return RESULT_OK;
          }
          
          // Command new speed for segment
          // (Except for point turns whose speeds are handled at the steering controller level)
          if (path_[currPathSegment_].GetType() != Planning::PST_POINT_TURN) {
            SpeedController::SetUserCommandedDesiredVehicleSpeed( path_[currPathSegment_].GetTargetSpeed() );
            SpeedController::SetUserCommandedAcceleration( path_[currPathSegment_].GetAccel() );
            SpeedController::SetUserCommandedDeceleration( path_[currPathSegment_].GetDecel() );
          }
#if(DEBUG_PATH_FOLLOWER)
          PRINT("*** PATH SEGMENT %d: speed = %f, accel = %f, decel = %f\n",
                currPathSegment_,
                path_[currPathSegment_].GetTargetSpeed(),
                path_[currPathSegment_].GetAccel(),
                path_[currPathSegment_].GetDecel());
#endif
          WheelController::ResetIntegralGainSums();
          
        }
        
        if (!DockingController::IsBusy()) {
          // Check that starting error is not too big
          // TODO: Check for excessive heading error as well?
          if (distToPath_mm_ > TOO_FAR_FROM_PATH_DIST_MM) {
            currPathSegment_ = -1;
#if(DEBUG_PATH_FOLLOWER)
            PRINT("PATH STARTING ERROR TOO LARGE (%f mm)\n", distToPath_mm_);
#endif
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
      
      
    } // namespace PathFollower
  } // namespace Cozmo
} // namespace Anki
