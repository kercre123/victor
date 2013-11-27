#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/debug.h"
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
      
      namespace { // Private Members
        typedef enum {
          PST_LINE,
          PST_ARC,
          PST_POINT_TURN
        } PathSegmentType;
        
        //typedef union {
        typedef struct {
          // Line
          struct s_line {
            Anki::Embedded::Point2f startPt;
            Anki::Embedded::Point2f endPt;
            f32 m;  // slope of line
            f32 b;  // y-intersect of line
            f32 dy_sign; // 1 if endPt.y - startPt.y >= 0
            Anki::Radians theta; // angle of line
            // TODO: Speed profile (by distance along line)
          } line;
          
          // Arc
          struct s_arc {
            Anki::Embedded::Point2f centerPt;
            f32 radius;
            Anki::Radians startRad;
            Anki::Radians endRad;
            bool movingCCW;
          } arc;
          
          // Point turn
          struct s_turn {
            float targetAngle;
            float maxAngularVel;
            float angularAccel;
            float angularDecel;
          } turn;
        } PathSegmentDef;
        
        typedef struct
        {
          PathSegmentType type;
          s16 desiredSpeed_mmPerSec;  // Desired speed during segment
          s16 terminalSpeed_mmPerSec; // Desired speed by the time we reach the end of the segment

          PathSegmentDef def;
          
        } PathSegment;
        
#define MAX_NUM_PATH_SEGMENTS 10
        PathSegment Path[MAX_NUM_PATH_SEGMENTS];
        s16 numPathSegments_ = 0;
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
        numPathSegments_ = 0;
        currPathSegment_ = -1;
#if(ENABLE_PATH_VIZ)
        Viz::ErasePath(0);
#endif
      } // Update()
      
      
      void EnablePathVisualization(bool on)
      {
        visualizePath_ = on;
      }
      
      
      // Add path segment
      // tODO: Change units to meters
      bool AppendPathSegment_Line(u32 matID, f32 x_start_m, f32 y_start_m, f32 x_end_m, f32 y_end_m)
      {
        if (numPathSegments_ >= MAX_NUM_PATH_SEGMENTS) {
          return FALSE;
        }
        
        Path[numPathSegments_].type = PST_LINE;
        Path[numPathSegments_].def.line.startPt.x = x_start_m;
        Path[numPathSegments_].def.line.startPt.y = y_start_m;
        Path[numPathSegments_].def.line.endPt.x = x_end_m;
        Path[numPathSegments_].def.line.endPt.y = y_end_m;
        
        // Pre-processed for convenience
        Path[numPathSegments_].def.line.m = (y_end_m - y_start_m) / (x_end_m - x_start_m);
        Path[numPathSegments_].def.line.b = y_start_m - Path[numPathSegments_].def.line.m * x_start_m;
        Path[numPathSegments_].def.line.dy_sign = ((y_end_m - y_start_m) >= 0) ? 1.0 : -1.0;
        Path[numPathSegments_].def.line.theta = atan2_fast(y_end_m - y_start_m, x_end_m - x_start_m);
        
        numPathSegments_++;
        
#if(ENABLE_PATH_VIZ)
        Viz::AppendPathSegmentLine(0, x_start_m, y_start_m, x_end_m, y_end_m);
#endif
        
        return TRUE;
      }
      
      
      bool AppendPathSegment_Arc(u32 matID, f32 x_center_m, f32 y_center_m, f32 radius_m, f32 startRad, f32 endRad)
      {
        if (numPathSegments_ >= MAX_NUM_PATH_SEGMENTS) {
          return FALSE;
        }
        
        // Assuming arcs are true functions.
        // i.e. for any value of x there can only be one y value.
        // This also means that an arc cannot sweep past the angles 0 or PI.
        assert( (SIGN(startRad) == SIGN(endRad)) ||
               (FLT_NEAR(startRad,0) || FLT_NEAR(endRad,0))
               );
        
        Path[numPathSegments_].type = PST_ARC;
        Path[numPathSegments_].def.arc.centerPt.x = x_center_m;
        Path[numPathSegments_].def.arc.centerPt.y = y_center_m;
        Path[numPathSegments_].def.arc.radius = radius_m;
        Path[numPathSegments_].def.arc.startRad = startRad;
        Path[numPathSegments_].def.arc.endRad = endRad;
        
        // Pre-processed for convenience
        Path[numPathSegments_].def.arc.movingCCW = endRad > startRad;
        
        numPathSegments_++;
        
#if(ENABLE_PATH_VIZ)
        Viz::AppendPathSegmentArc(0, x_center_m, y_center_m,
                                  radius_m, startRad, endRad);
#endif
        
        return TRUE;
      }
      
      
      bool AppendPathSegment_PointTurn(u32 matID, f32 targetAngle, f32 maxAngularVel, f32 angularAccel, f32 angularDecel)
      {
        if (numPathSegments_ >= MAX_NUM_PATH_SEGMENTS) {
          return FALSE;
        }
        
        Path[numPathSegments_].type = PST_POINT_TURN;
        Path[numPathSegments_].def.turn.targetAngle = targetAngle;
        Path[numPathSegments_].def.turn.maxAngularVel = maxAngularVel;
        Path[numPathSegments_].def.turn.angularAccel = angularAccel;
        Path[numPathSegments_].def.turn.angularDecel = angularDecel;
        
        numPathSegments_++;
        
        return TRUE;
      }
      
      
      int GetNumPathSegments(void)
      {
        return numPathSegments_;
      }
      
      
      bool StartPathTraversal()
      {
        // Set first path segment
        if (numPathSegments_ > 0) {
          currPathSegment_ = 0;
          wasInRange_ = false;
          //Robot::SetOperationMode(Robot::FOLLOW_PATH);
          SteeringController::SetPathFollowMode();
        }
        
        // Visualize path
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
        PathSegmentDef::s_line* currSeg = &(Path[currPathSegment_].def.line);
        
        // Find shortest path to current segment.
        // Shortest path is along a line with inverse negative slope (i.e. -1/m).
        // Point of intersection is solution to mx + b == (-1/m)*x + b_inv where b_inv = y-(-1/m)*x
        f32 x, y;
        Radians angle;
        Localization::GetCurrentMatPose(x, y, angle);
        
#if(DEBUG_PATH_FOLLOWER)
        PERIODIC_PRINT(DBG_PERIOD, "currPathSeg: %d, LINE (%f, %f, %f, %f)\n", currPathSegment_, currSeg->startPt.x, currSeg->startPt.y, currSeg->endPt.x, currSeg->endPt.y);
        PERIODIC_PRINT(DBG_PERIOD, "Robot Pose: x: %f, y: %f ang: %f\n", x,y,angle.ToFloat());
#endif
        
        
        // Distance to start point
        f32 sqDistToStartPt = (currSeg->startPt.x - x) * (currSeg->startPt.x - x) +
                              (currSeg->startPt.y - y) * (currSeg->startPt.y - y);
        
        // Distance to end point
        f32 sqDistToEndPt = (currSeg->endPt.x - x) * (currSeg->endPt.x - x) +
                            (currSeg->endPt.y - y) * (currSeg->endPt.y - y);
        
        
        if (FLT_NEAR(currSeg->startPt.x, currSeg->endPt.x)) {
          // Special case: Vertical line
          if (currSeg->endPt.y > currSeg->startPt.y) {
            shortestDistanceToPath_m = currSeg->startPt.x - x;
          } else {
            shortestDistanceToPath_m = x - currSeg->startPt.x;
          }
          
          // Compute angle difference
          radDiff = (currSeg->theta - angle).ToFloat();
          
          // If the point (x_intersect,y_intersect) is not between startPt and endPt,
          // and the robot is closer to the end point than it is to the start point,
          // then we've passed this segment and should go to next one
          if ( SIGN(currSeg->startPt.y - y) == SIGN(currSeg->endPt.y - y)
              && (sqDistToStartPt > sqDistToEndPt) ) {
            return FALSE;
          }

        } else if (FLT_NEAR(currSeg->startPt.y, currSeg->endPt.y)) {
          // Special case: Horizontal line
          if (currSeg->endPt.x > currSeg->startPt.x) {
            shortestDistanceToPath_m = y - currSeg->startPt.y;
          } else {
            shortestDistanceToPath_m = currSeg->startPt.y - y;
          }
          
          // Compute angle difference
          radDiff = (currSeg->theta - angle).ToFloat();

          // If the point (x_intersect,y_intersect) is not between startPt and endPt,
          // and the robot is closer to the end point than it is to the start point,
          // then we've passed this segment and should go to next one
          if ( SIGN(currSeg->startPt.x - x) == SIGN(currSeg->endPt.x - x)
              && (sqDistToStartPt > sqDistToEndPt) ) {
            return FALSE;
          }
          
        } else {
          
          f32 m = currSeg->m;
          f32 b = currSeg->b;
          
          f32 b_inv = y + x/m;
          
          f32 x_intersect = m * (b_inv - b) / (m*m + 1);
          f32 y_intersect = - (x_intersect / m) + b_inv;
          
          f32 dy = y - y_intersect;
          f32 dx = x - x_intersect;
          
          shortestDistanceToPath_m = sqrtf(dy*dy + dx*dx);
          
#if(DEBUG_PATH_FOLLOWER)
          PERIODIC_PRINT(DBG_PERIOD, "m: %f, b: %f\n",m,b);
          PERIODIC_PRINT(DBG_PERIOD, "x_int: %f, y_int: %f, b_inv: %f\n", x_intersect, y_intersect, b_inv);
          PERIODIC_PRINT(DBG_PERIOD, "dy: %f, dx: %f, dist: %f\n", dy, dx, shortestDistanceToPath_m);
          PERIODIC_PRINT(DBG_PERIOD, "SIGN(dx): %d, dy_sign: %f\n", (SIGN(dx) ? 1 : -1), currSeg->dy_sign);
          PERIODIC_PRINT(DBG_PERIOD, "lineTheta: %f\n", currSeg->theta.ToFloat());
          //PRINT("lineTheta: %f, robotTheta: %f\n", currSeg->theta.ToFloat(), currPose.get_angle().ToFloat());
#endif
          
          // Compute the sign of the error distance 
          shortestDistanceToPath_m *= (SIGN(m) ? 1 : -1) * (SIGN(dy) ? 1 : -1) * currSeg->dy_sign;
          
          
          // Compute angle difference
          radDiff = (currSeg->theta - angle).ToFloat();
          
          
          // Did we pass the current segment?
          // If the point (x_intersect,y_intersect) is not between startPt and endPt,
          // and the robot is closer to the end point than it is to the start point,
          // then we've passed this segment and should go to next one
          if ( (SIGN(currSeg->startPt.x - x_intersect) == SIGN(currSeg->endPt.x - x_intersect))
              && (SIGN(currSeg->startPt.y - y_intersect) == SIGN(currSeg->endPt.y - y_intersect))
              && (sqDistToStartPt > sqDistToEndPt)
              ) {
            return FALSE;
          }
        }
        
        return TRUE;
      }
      
      
      bool ProcessPathSegmentArc(f32 &shortestDistanceToPath_m, f32 &radDiff)
      {
        PathSegmentDef::s_arc* currSeg = &(Path[currPathSegment_].def.arc);
        
#if(DEBUG_PATH_FOLLOWER)
        PRINT("currPathSeg: %d, ARC (%f, %f), startRad: %f, endRad: %f, radius: %f\n",
               currPathSegment_, currSeg->centerPt.x, currSeg->centerPt.y, currSeg->startRad.ToFloat(), currSeg->endRad.ToFloat(), currSeg->radius);
#endif
        
        f32 x, y;
        Radians angle;
        Localization::GetCurrentMatPose(x, y, angle);
        
        
        // Assuming arc is broken up so that it is a true function
        
        // Arc paramters
        f32 x_center = currSeg->centerPt.x;
        f32 y_center = currSeg->centerPt.y;
        f32 r = currSeg->radius;
        Anki::Radians startRad = currSeg->startRad;
        Anki::Radians endRad = currSeg->endRad;
        
        // Line formed by circle center and robot pose
        f32 dy = y - y_center;
        f32 dx = x - x_center;
        f32 m = dy / dx;
        f32 b = y - m*x;
        
        
        // Find heading error
        Anki::Radians theta_line = atan2_fast(dy,dx); // angle of line from circle center to robot
        Anki::Radians theta_tangent = theta_line + Anki::Radians((currSeg->movingCCW ? 1 : -1 ) * PIDIV2);
        
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
 
        }
        
        // Figure out sign of distance according to robot orientation and whether it's inside or outside the circle.
        f32 arcSweep = (currSeg->endRad - currSeg->startRad).ToFloat();
        bool robotInsideCircle = ABS(dx) < ABS(x_intersect - x_center);
        if ((robotInsideCircle && arcSweep < 0) || (!robotInsideCircle && arcSweep > 0)) {
          shortestDistanceToPath_m *= -1;
        }
        
        

        
#if(DEBUG_PATH_FOLLOWER)
        PRINT("A: %f, B: %f, C: %f, sqrt: %f\n", A, B, C, sqrtPart);
        PRINT("x: %f, y: %f, m: %f, b: %f\n", x,y,m,b);
        PRINT("x_center: %f, y_center: %f\n", x_center, y_center);
        //PRINT("x_int: %f, y_int: %f, x's: (%f %f), alternate y_int: %f\n", x_intersect, y_intersect, x_intersect_1, x_intersect_2, y_int_alt);
        PRINT("x_int: %f, y_int: %f, x's: (%f %f)\n", x_intersect, y_intersect, x_intersect_1, x_intersect_2);
        PRINT("dy: %f, dx: %f, dist: %f, radDiff: %f\n", dy, dx, shortestDistanceToPath_m, radDiff);
        PRINT("arcSweep: %f, insideCircle: %d\n", arcSweep, robotInsideCircle);
        PRINT("theta_line: %f, theta_tangent: %f\n", theta_line.ToFloat(), theta_tangent.ToFloat());
#endif
        
        // Did we pass the current segment?
        if ( (currSeg->movingCCW && (theta_line - currSeg->startRad > arcSweep)) ||
            (!currSeg->movingCCW && (theta_line - currSeg->startRad < arcSweep)) ){
          return FALSE;
        }
        
        
        return TRUE;
      }
      

      bool ProcessPathSegmentPointTurn(f32 &shortestDistanceToPath_m, f32 &radDiff)
      {
        PathSegmentDef::s_turn* currSeg = &(Path[currPathSegment_].def.turn);
        
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
        
        switch (Path[currPathSegment_].type) {
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
          if (++currPathSegment_ >= numPathSegments_) {
            // Path is complete
            PathComplete();
            return EXIT_SUCCESS;
          }
#if(DEBUG_PATH_FOLLOWER)
          PRINT("PATH SEGMENT %d\n", currPathSegment_);
#endif
        }
        
        wasInRange_ = inRange;
        
        
        // Check that starting error is not too big
        // TODO: Check for excessive heading error as well?
        if (distToPath_m_ > 0.05) {
          currPathSegment_ = -1;
#if(DEBUG_PATH_FOLLOWER)
          PRINT("PATH STARTING ERROR TOO LARGE %f\n", distToPath_m_);
#endif
          return EXIT_FAILURE;
        }
        
        return EXIT_SUCCESS;
      }
      
      
    } // namespace PathFollower
  } // namespace Cozmo
} // namespace Anki
