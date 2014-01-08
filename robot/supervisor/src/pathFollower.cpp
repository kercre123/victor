#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/debug.h"
#include "dockingController.h"
#include "anki/cozmo/robot/path.h"
#include "anki/cozmo/robot/pathFollower.h"
#include "anki/cozmo/robot/localization.h"
#include "anki/cozmo/robot/steeringController.h"
#include "anki/cozmo/robot/wheelController.h"
#include "anki/cozmo/robot/speedController.h"

#include "anki/common/robot/utilities_c.h"
#include "anki/common/robot/trig_fast.h"

#include "anki/cozmo/robot/hal.h"

#if(DEBUG_MAIN_EXECUTION)
#include "sim_overlayDisplay.h"
#endif

#ifndef SIMULATOR
#define ENABLE_PATH_VIZ 0
#else
#define ENABLE_PATH_VIZ 1
#endif

// The number of tics desired in between debug prints
#define DBG_PERIOD 200

#if ENABLE_PATH_VIZ
#include "sim_pathFollower.h"
#endif

namespace Anki
{
  namespace Cozmo
  {
    namespace PathFollower
    {
      
      namespace {
        // ======== Pre-processed path segment info =======
        
        // Line
        f32 line_m_;  // slope of line
        f32 line_b_;  // y-intersect of line
        f32 line_dy_sign_; // 1 if endPt.y - startPt.y >= 0
        Radians line_theta_; // angle of line
        
        f32 arc_angTraversed_;
        // ===== End of pre-processed path segment info ===
        
        const f32 LOOK_AHEAD_DIST_M = 0.01f;

        Path path_;
        s16 currPathSegment_ = -1;
        
        // Shortest distance to path
        f32 distToPath_m_ = 0;
        
        // Angular error with path
        f32 radToPath_ = 0;
        
        bool pointTurnStarted_ = false;
        
        bool visualizePath_ = TRUE;
        
      } // Private Members
      
      
      ReturnCode Init(void)
      {
        ClearPath();
        
#if ENABLE_PATH_VIZ
        if(Viz::Init() == EXIT_FAILURE) {
          PRINT("PathFollower visualization init failed.\n");
          return EXIT_FAILURE;
        }
#endif
      
        return EXIT_SUCCESS;
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
      
      
      void EnablePathVisualization(bool on)
      {
        visualizePath_ = on;
      }
      
      
      void SetPathForViz() {
#if(ENABLE_PATH_VIZ)
        Viz::ErasePath(0);
        for (u8 i=0; i<path_.GetNumSegments(); ++i) {
          switch(path_[i].type) {
            case PST_LINE:
              Viz::AppendPathSegmentLine(0,
                                         path_[i].def.line.startPt_x,
                                         path_[i].def.line.startPt_y,
                                         path_[i].def.line.endPt_x,
                                         path_[i].def.line.endPt_y);
              break;
            case PST_ARC:
              Viz::AppendPathSegmentArc(0,
                                        path_[i].def.arc.centerPt_x,
                                        path_[i].def.arc.centerPt_y,
                                        path_[i].def.arc.radius,
                                        path_[i].def.arc.startRad,
                                        path_[i].def.arc.sweepRad);
              break;
            default:
              break;
          }
        }
#endif
      }
      
      
      
      // Add path segment
      // TODO: Change units to meters
      bool AppendPathSegment_Line(u32 matID, f32 x_start_m, f32 y_start_m, f32 x_end_m, f32 y_end_m)
      {
        return path_.AppendLine(matID, x_start_m, y_start_m, x_end_m, y_end_m);
      }
      
      
      bool AppendPathSegment_Arc(u32 matID, f32 x_center_m, f32 y_center_m, f32 radius_m, f32 startRad, f32 sweepRad)
      {
        return path_.AppendArc(matID, x_center_m, y_center_m, radius_m, startRad, sweepRad);
      }
      
      
      bool AppendPathSegment_PointTurn(u32 matID, f32 x, f32 y, f32 targetAngle, f32 maxAngularVel, f32 angularAccel, f32 angularDecel)
      {
        return path_.AppendPointTurn(matID, x, y, targetAngle, maxAngularVel, angularAccel, angularDecel);
      }
      
      u8 GenerateDubinsPath(f32 start_x, f32 start_y, f32 start_theta,
                            f32 end_x, f32 end_y, f32 end_theta,
                            f32 start_radius, f32 end_radius,
                            f32 final_straight_approach_length,
                            f32 *path_length)
      {
        return path_.GenerateDubinsPath(start_x, start_y, start_theta,
                                        end_x, end_y, end_theta,
                                        start_radius, end_radius,
                                        final_straight_approach_length,
                                        path_length);
      }
      
      
      void PreProcessPathSegment(const PathSegment &segment)
      {
        switch(segment.type) {
          case PST_LINE:
          {
            const PathSegmentDef::s_line* l = &(segment.def.line);
            line_m_ = (l->endPt_y - l->startPt_y) / (l->endPt_x - l->startPt_x);
            line_b_ = l->startPt_y - line_m_ * l->startPt_x;
            line_dy_sign_ = ((l->endPt_y - l->startPt_y) >= 0) ? 1.0 : -1.0;
            line_theta_ = atan2_fast(l->endPt_y - l->startPt_y, l->endPt_x - l->startPt_x);
            break;
          }
          case PST_ARC:
            arc_angTraversed_ = 0;
            break;
          case PST_POINT_TURN:
          default:
            break;
        }
      }


      u8 GetClosestSegment(const f32 x, const f32 y, const f32 angle)
      {
        assert(path_.GetNumSegments() > 0);
        
        SegmentRangeStatus res;
        f32 distToSegment, angError;
        
        u8 closestSegId = 0;
        SegmentRangeStatus closestSegmentRangeStatus = OOR_NEAR_END;
        f32 distToClosestSegment = FLT_MAX;
        
        for (u8 i=0; i<path_.GetNumSegments(); ++i) {
          res = path_[i].GetDistToSegment(x,y,angle,distToSegment,angError);
#if(DEBUG_PATH_FOLLOWER)
          PRINT("PathDist: %f  (res=%d)\n", distToSegment, res);
#endif
          if (ABS(distToSegment) < distToClosestSegment && (res == IN_SEGMENT_RANGE || res == OOR_NEAR_START)) {
            closestSegId = i;
            distToClosestSegment = ABS(distToSegment);
            closestSegmentRangeStatus = res;
#if(DEBUG_PATH_FOLLOWER)
            PRINT(" New closest seg: %d, distToSegment %f (res=%d)\n", i, distToSegment, res);
#endif
          }
        }
        
        return closestSegId;
      }
      
      
      bool StartPathTraversal()
      {
        // Set first path segment
        if (path_.GetNumSegments() > 0) {
          
          path_.PrintPath();
          
          if (!path_.CheckContinuity()) {
            PRINT("ERROR: Path is discontinuous\n");
            return false;
          }
          
          
          // Get current robot pose
          f32 x, y;
          Radians angle;
          Localization::GetCurrentMatPose(x, y, angle);
          
          //currPathSegment_ = 0;
          currPathSegment_ = GetClosestSegment(x,y,angle.ToFloat());
          
          
          PreProcessPathSegment(path_[currPathSegment_]);

          //Robot::SetOperationMode(Robot::FOLLOW_PATH);
          SteeringController::SetPathFollowMode();
        }
        
        // Visualize path
        SetPathForViz();
        if (visualizePath_) {
#if(ENABLE_PATH_VIZ)
          Viz::ShowPath(0, true);
#endif
        }
        
        return TRUE;
      }
      
      
      bool IsTraversingPath()
      {
        return currPathSegment_ >= 0;
      }
      
      
      SegmentRangeStatus ProcessPathSegment(f32 &shortestDistanceToPath_m, f32 &radDiff)
      {
        // Get current robot pose
        f32 x, y;
        Radians angle;
        Localization::GetCurrentMatPose(x, y, angle);
        

        // Compute lookahead position
        if (LOOK_AHEAD_DIST_M != 0) {
          x += LOOK_AHEAD_DIST_M * cosf(angle.ToFloat());
          y += LOOK_AHEAD_DIST_M * sinf(angle.ToFloat());
        }
        
      
        return path_[currPathSegment_].GetDistToSegment(x,y,angle.ToFloat(),shortestDistanceToPath_m,radDiff);
      }
      
      

      SegmentRangeStatus ProcessPathSegmentPointTurn(f32 &shortestDistanceToPath_m, f32 &radDiff)
      {
        const PathSegmentDef::s_turn* currSeg = &(path_[currPathSegment_].def.turn);
        
#if(DEBUG_PATH_FOLLOWER)
        Radians currOrientation = Localization::GetCurrentMatOrientation();
        PRINT("currPathSeg: %d, TURN  currAngle: %f, targetAngle: %f, maxAngVel: %f, angAccel: %f, angDecel: %f\n",
               currPathSegment_, currOrientation.ToFloat(), currSeg->targetAngle, currSeg->maxAngularVel, currSeg->angularAccel, currSeg->angularDecel);
#endif
        
        // When the car is stopped, initiate point turn
        if (!pointTurnStarted_) {
#if(DEBUG_PATH_FOLLOWER)
          PRINT("EXECUTE POINT TURN\n");
#endif
          SteeringController::ExecutePointTurn(currSeg->targetAngle,
                                               currSeg->maxAngularVel,
                                               currSeg->angularAccel,
                                               currSeg->angularDecel);
          pointTurnStarted_ = true;

        } else {
          if (SteeringController::GetMode() != SteeringController::SM_POINT_TURN) {
            pointTurnStarted_ = false;
            return OOR_NEAR_END;
          }
        }

        
        return IN_SEGMENT_RANGE;
      }
      
      // Post-path completion cleanup
      void PathComplete()
      {
        pointTurnStarted_ = false;
        currPathSegment_ = -1;
#if(DEBUG_PATH_FOLLOWER)
        PRINT("PATH COMPLETE\n");
#endif
#if(ENABLE_PATH_VIZ)
        Viz::ErasePath(0);
#endif
      }
      
      
      bool GetPathError(f32 &shortestDistanceToPath_m, f32 &radDiff)
      {
        if (!IsTraversingPath()) {
          return false;
        }
        
        shortestDistanceToPath_m = distToPath_m_;
        radDiff = radToPath_;
        return true;
      }
      
      
      
      ReturnCode Update()
      {
        
#if(FREE_DRIVE_DUBINS_TEST)
        // Test code to visualize Dubins path online
        f32 start_x,start_y;
        Radians start_theta;
        Localization::GetCurrentMatPose(start_x,start_y,start_theta);
        path_.Clear();
        
        const f32 end_x = 0.0;
        const f32 end_y = 0.25;
        const f32 end_theta = 0.5*PI;
        const f32 start_radius = 0.05;
        const f32 end_radius = 0.05;
        const f32 final_straight_approach_length = 0.1;
        f32 path_length;
        u8 numSegments = path_.GenerateDubinsPath(start_x, start_y, start_theta.ToFloat(),
                                                  end_x, end_y, end_theta,
                                                  start_radius, end_radius,
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
          return EXIT_FAILURE;
        }
        
        SegmentRangeStatus segRes = OOR_NEAR_END;
        switch (path_[currPathSegment_].type) {
          case PST_LINE:
          case PST_ARC:
            segRes = ProcessPathSegment(distToPath_m_, radToPath_);
            break;
          case PST_POINT_TURN:
            segRes = ProcessPathSegmentPointTurn(distToPath_m_, radToPath_);
            break;
          default:
            // TODO: Error?
            break;
        }
        
#if(DEBUG_PATH_FOLLOWER)
        PERIODIC_PRINT(DBG_PERIOD,"PATH ERROR: %f m, %f deg\n", distToPath_m_, RAD_TO_DEG(radToPath_));
#endif
        
        // Go to next path segment if no longer in range of the current one
        if (segRes == OOR_NEAR_END) {
          if (++currPathSegment_ >= path_.GetNumSegments()) {
            // Path is complete
            PathComplete();
            return EXIT_SUCCESS;
          }
          
          PreProcessPathSegment(path_[currPathSegment_]);
          
          
#if(DEBUG_PATH_FOLLOWER)
          PRINT("PATH SEGMENT %d\n", currPathSegment_);
#endif
        }
        
        if (!DockingController::IsDockingOrPlacing()) {
          // Check that starting error is not too big
          // TODO: Check for excessive heading error as well?
          if (distToPath_m_ > 0.05) {
            currPathSegment_ = -1;
  #if(DEBUG_PATH_FOLLOWER)
            PRINT("PATH STARTING ERROR TOO LARGE %f\n", distToPath_m_);
  #endif
            return EXIT_FAILURE;
          }
        }
        
        return EXIT_SUCCESS;
      }
      
      
      void PrintPathSegment(s16 segment)
      {
        path_.PrintSegment(segment);
      }
      
      
    } // namespace PathFollower
  } // namespace Cozmo
} // namespace Anki
