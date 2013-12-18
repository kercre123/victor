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
        

        Path path_;
        s16 currPathSegment_ = -1;
        
        // Shortest distance to path
        f32 distToPath_m_ = 0;
        
        // Angular error with path
        f32 radToPath_ = 0;
        
        bool pointTurnStarted_ = false;
        
        bool visualizePath_ = TRUE;
        
        // Whether or not the robot was within range of the
        // current path segment in the previous tic.
        bool wasInRange_ = false;
        
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
      
      
      bool StartPathTraversal()
      {
        // Set first path segment
        if (path_.GetNumSegments() > 0) {
          
          path_.PrintPath();
          
          if (!path_.CheckContinuity()) {
            PRINT("ERROR: Path is discontinuous\n");
            return false;
          }
          
          currPathSegment_ = 0;
          PreProcessPathSegment(path_[currPathSegment_]);
          wasInRange_ = false;
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
      
      
      bool ProcessPathSegmentLine(f32 &shortestDistanceToPath_m, f32 &radDiff)
      {
        const PathSegmentDef::s_line* currSeg = &(path_[currPathSegment_].def.line);
        
        // Find shortest path to current segment.
        // Shortest path is along a line with inverse negative slope (i.e. -1/m).
        // Point of intersection is solution to mx + b == (-1/m)*x + b_inv where b_inv = y-(-1/m)*x
        
        // Get current robot pose
        f32 x, y;
        Radians angle;
        Localization::GetCurrentMatPose(x, y, angle);
        
        
#if(DEBUG_PATH_FOLLOWER)
        PERIODIC_PRINT(DBG_PERIOD, "currPathSeg: %d, LINE (%f, %f, %f, %f)\n", currPathSegment_, currSeg->startPt_x, currSeg->startPt_y, currSeg->endPt_x, currSeg->endPt_y);
        PERIODIC_PRINT(DBG_PERIOD, "Robot Pose: x: %f, y: %f ang: %f\n", x,y,angle.ToFloat());
#endif
        
        
        // Distance to start point
        f32 sqDistToStartPt = (currSeg->startPt_x - x) * (currSeg->startPt_x - x) +
                              (currSeg->startPt_y - y) * (currSeg->startPt_y - y);
        
        // Distance to end point
        f32 sqDistToEndPt = (currSeg->endPt_x - x) * (currSeg->endPt_x - x) +
                            (currSeg->endPt_y - y) * (currSeg->endPt_y - y);
        
        
        if (FLT_NEAR(currSeg->startPt_x, currSeg->endPt_x)) {
          // Special case: Vertical line
          if (currSeg->endPt_y > currSeg->startPt_y) {
            shortestDistanceToPath_m = currSeg->startPt_x - x;
          } else {
            shortestDistanceToPath_m = x - currSeg->startPt_x;
          }
          
          // Compute angle difference
          radDiff = (line_theta_ - angle).ToFloat();
          
          // If the point (x_intersect,y_intersect) is not between startPt and endPt,
          // and the robot is closer to the end point than it is to the start point,
          // then we've passed this segment and should go to next one
          if ( SIGN(currSeg->startPt_y - y) == SIGN(currSeg->endPt_y - y)
              && (sqDistToStartPt > sqDistToEndPt) ) {
            return FALSE;
          }

        } else if (FLT_NEAR(currSeg->startPt_y, currSeg->endPt_y)) {
          // Special case: Horizontal line
          if (currSeg->endPt_x > currSeg->startPt_x) {
            shortestDistanceToPath_m = y - currSeg->startPt_y;
          } else {
            shortestDistanceToPath_m = currSeg->startPt_y - y;
          }
          
          // Compute angle difference
          radDiff = (line_theta_ - angle).ToFloat();

          // If the point (x_intersect,y_intersect) is not between startPt and endPt,
          // and the robot is closer to the end point than it is to the start point,
          // then we've passed this segment and should go to next one
          if ( SIGN(currSeg->startPt_x - x) == SIGN(currSeg->endPt_x - x)
              && (sqDistToStartPt > sqDistToEndPt) ) {
            return FALSE;
          }
          
        } else {
          
          f32 b_inv = y + x/line_m_;
          
          f32 x_intersect = line_m_ * (b_inv - line_b_) / (line_m_*line_m_ + 1);
          f32 y_intersect = - (x_intersect / line_m_) + b_inv;
          
          f32 dy = y - y_intersect;
          f32 dx = x - x_intersect;
          
          shortestDistanceToPath_m = sqrtf(dy*dy + dx*dx);
          
#if(DEBUG_PATH_FOLLOWER)
          PERIODIC_PRINT(DBG_PERIOD, "m: %f, b: %f\n",line_m_,line_b_);
          PERIODIC_PRINT(DBG_PERIOD, "x_int: %f, y_int: %f, b_inv: %f\n", x_intersect, y_intersect, b_inv);
          PERIODIC_PRINT(DBG_PERIOD, "dy: %f, dx: %f, dist: %f\n", dy, dx, shortestDistanceToPath_m);
          PERIODIC_PRINT(DBG_PERIOD, "SIGN(dx): %d, dy_sign: %f\n", (SIGN(dx) ? 1 : -1), line_dy_sign_);
          PERIODIC_PRINT(DBG_PERIOD, "lineTheta: %f\n", line_theta_.ToFloat());
          //PRINT("lineTheta: %f, robotTheta: %f\n", currSeg->theta.ToFloat(), currPose.get_angle().ToFloat());
#endif
          
          // Compute the sign of the error distance 
          shortestDistanceToPath_m *= (SIGN(line_m_) ? 1 : -1) * (SIGN(dy) ? 1 : -1) * line_dy_sign_;
          
          
          // Compute angle difference
          radDiff = (line_theta_ - angle).ToFloat();
          
          
          // Did we pass the current segment?
          // If the point (x_intersect,y_intersect) is not between startPt and endPt,
          // and the robot is closer to the end point than it is to the start point,
          // then we've passed this segment and should go to next one
          if ( (SIGN(currSeg->startPt_x - x_intersect) == SIGN(currSeg->endPt_x - x_intersect))
              && (SIGN(currSeg->startPt_y - y_intersect) == SIGN(currSeg->endPt_y - y_intersect))
              && (sqDistToStartPt > sqDistToEndPt)
              ) {
            return FALSE;
          }
        }
        
        return TRUE;
      }
      
      
      bool ProcessPathSegmentArc(f32 &shortestDistanceToPath_m, f32 &radDiff)
      {
        const PathSegmentDef::s_arc* currSeg = &(path_[currPathSegment_].def.arc);
        
#if(DEBUG_PATH_FOLLOWER)
        PRINT("currPathSeg: %d, ARC (%f, %f), startRad: %f, sweepRad: %f, radius: %f\n",
               currPathSegment_, currSeg->centerPt_x, currSeg->centerPt_y, currSeg->startRad, currSeg->sweepRad, currSeg->radius);
#endif
        
        // Get current robot pose
        f32 x, y;
        Radians angle;
        Localization::GetCurrentMatPose(x, y, angle);
        
        
        // Assuming arc is broken up so that it is a true function
        
        // Arc paramters
        f32 x_center = currSeg->centerPt_x;
        f32 y_center = currSeg->centerPt_y;
        f32 r = currSeg->radius;
        Anki::Radians startRad = currSeg->startRad;
        
        // Line formed by circle center and robot pose
        f32 dy = y - y_center;
        f32 dx = x - x_center;
        f32 m = dy / dx;
        f32 b = y - m*x;
        
        
        // Find heading error
        bool movingCCW = currSeg->sweepRad >= 0;
        Anki::Radians theta_line = atan2_fast(dy,dx); // angle of line from circle center to robot
        Anki::Radians theta_tangent = theta_line + Anki::Radians((movingCCW ? 1 : -1 ) * PIDIV2);
        
        radDiff = (theta_tangent - angle).ToFloat();
        
        // If the line is nearly vertical (within 0.5deg), approximate it
        // with true vertical so we don't take sqrts of -ve numbers.
        f32 x_intersect, y_intersect;
        if (NEAR(ABS(theta_line.ToFloat()), PIDIV2, 0.01)) {
          shortestDistanceToPath_m = ABS(dy) - r;
          x_intersect = x_center;
          y_intersect = y_center + r * (dx > 0 ? 1 : -1);
          
        } else {
          
          // Where does circle (x - x_center)^2 + (y - y_center)^2 = r^2 and y=mx+b intersect where y=mx+b represents
          // the line between the circle center and the robot?
          // (y - y_center)^2 == r^2 - (x - x_center)^2
          // y = sqrt(r^2 - (x - x_center)^2) + y_center
          //   = mx + b
          // (mx + b - y_center)^2 == r^2 - (x - x_center)^2
          // m^2*x^2 + 2*m*(b - y_center)*x + (b - y_center)^2 == r^2 - x^2 + 2*x_center*x - x_center^2
          // (m^2+1) * x^2 + (2*m*(b - y_center) - 2*x_center) * x + (b - y_center)^2 - r^2 + x_center^2 == 0
          //
          // Use quadratic formula to solve
          
          // Quadratic formula coefficients
          f32 A = m*m + 1;
          f32 B = 2*m*(b-y_center) - 2*x_center;
          f32 C = (b - y_center)*(b - y_center) - r*r + x_center*x_center;
          f32 sqrtPart = sqrtf(B*B - 4*A*C);
          
          f32 x_intersect_1 = (-B + sqrtPart) / (2*A);
          f32 x_intersect_2 = (-B - sqrtPart) / (2*A);
          
          
          // Now we have 2 roots.
          // Select the one that's on the same side of the circle center as the robot is.
          x_intersect = x_intersect_2;
          if (SIGN(dx) == SIGN(x_intersect_1 - x_center)) {
            x_intersect = x_intersect_1;
          }
          
          // Find y value of intersection
          y_intersect = y_center + (dy > 0 ? 1 : -1) * sqrtf(r*r - (x_intersect - x_center) * (x_intersect - x_center));
          
          // Compute distance to intersection point (i.e. shortest distance to arc)
          shortestDistanceToPath_m = sqrtf((x - x_intersect) * (x - x_intersect) + (y - y_intersect) * (y - y_intersect));

#if(DEBUG_PATH_FOLLOWER)
          PRINT("A: %f, B: %f, C: %f, sqrt: %f\n", A, B, C, sqrtPart);
          PRINT("x_intersects: (%f %f)\n", x_intersect_1, x_intersect_2);
#endif

          
        }
        
        // Figure out sign of distance according to robot orientation and whether it's inside or outside the circle.
        bool robotInsideCircle = ABS(dx) < ABS(x_intersect - x_center);
        if ((robotInsideCircle && !movingCCW) || (!robotInsideCircle && movingCCW)) {
          shortestDistanceToPath_m *= -1;
        }
        
        

        
#if(DEBUG_PATH_FOLLOWER)
        PRINT("x: %f, y: %f, m: %f, b: %f\n", x,y,m,b);
        PRINT("x_center: %f, y_center: %f\n", x_center, y_center);
        PRINT("x_int: %f, y_int: %f\n", x_intersect, y_intersect);
        PRINT("dy: %f, dx: %f, dist: %f, radDiff: %f\n", dy, dx, shortestDistanceToPath_m, radDiff);
        PRINT("insideCircle: %d\n", robotInsideCircle);
        PRINT("theta_line: %f, theta_tangent: %f\n", theta_line.ToFloat(), theta_tangent.ToFloat());
#endif
        
        // Did we pass the current segment?
        // Check if the angDiff exceeds the sweep angle.
        // Also check for transitions between -PI and +PI by checking if angDiff
        // ever exceeds a conservative half the distance if PI was approached from the opposite direction.
        f32 angDiff = (theta_line - startRad).ToFloat();
        if ( (movingCCW && (angDiff > currSeg->sweepRad || angDiff < -0.5*(2*PI-currSeg->sweepRad))) ||
            (!movingCCW && (angDiff < currSeg->sweepRad || angDiff >  0.5*(2*PI+currSeg->sweepRad))) ){
          return FALSE;
        }
        
        
        return TRUE;
      }
      

      bool ProcessPathSegmentPointTurn(f32 &shortestDistanceToPath_m, f32 &radDiff)
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
            return false;
          }
        }

        
        return true;
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
        if (currPathSegment_ < 0) {
          SpeedController::SetUserCommandedDesiredVehicleSpeed(0);
          return EXIT_FAILURE;
        }
        
        bool inRange = false;
        
        switch (path_[currPathSegment_].type) {
          case PST_LINE:
            inRange = ProcessPathSegmentLine(distToPath_m_, radToPath_);
            break;
          case PST_ARC:
            inRange = ProcessPathSegmentArc(distToPath_m_, radToPath_);
            break;
          case PST_POINT_TURN:
            inRange = ProcessPathSegmentPointTurn(distToPath_m_, radToPath_);
            break;
          default:
            // TODO: Error?
            break;
        }
        
#if(DEBUG_PATH_FOLLOWER)
        PERIODIC_PRINT(DBG_PERIOD,"PATH ERROR: %f m, %f deg\n", distToPath_m_, RAD_TO_DEG(radToPath_));
#endif
        
        // Go to next path segment if no longer in range of the current one
        if (!inRange && wasInRange_) {
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
        
        wasInRange_ = inRange;
        
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
