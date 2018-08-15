#include "coretech/planning/shared/path.h"
#include "coretech/common/shared/radians.h"
#include "coretech/common/shared/utilities_shared.h"
#include "util/math/math.h"
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>

#define DEBUG_PATH 0

#ifdef CORETECH_ENGINE
#define DEBUG_PATH_APPEND 0
#include <stdio.h>
#include <float.h>
#define ATAN2_FAST(y,x) atan2(y,x)
#define ATAN2_ACC(y,x) atan2(y,x)
#elif defined CORETECH_ROBOT
#define DEBUG_PATH_APPEND 0
#include "coretech/common/robot/utilities_c.h"
#define ATAN2_FAST(y,x) atan2(y,x)
#define ATAN2_ACC(y,x) atan2(y,x)
#endif


namespace Anki
{
  namespace Planning
  {

    ////////////// PathSegment implementations ////////////
    void PathSegment::DefineLine(f32 x_start, f32 y_start, f32 x_end, f32 y_end,
                                 f32 targetSpeed, f32 accel, f32 decel)
    {
      type_ = PST_LINE;
      def_.line.startPt_x = x_start;
      def_.line.startPt_y = y_start;
      def_.line.endPt_x = x_end;
      def_.line.endPt_y = y_end;
      
      // Compute end angle
      // If going backwards, flip it 180
      def_.line.endAngle = Radians( ATAN2_ACC(y_end - y_start, x_end - x_start) + (targetSpeed < 0 ? M_PI_F : 0) ).ToFloat();
      
      SetSpeedProfile(targetSpeed, accel, decel);
    }
    
    void PathSegment::DefineArc(f32 x_center, f32 y_center, f32 radius, f32 startRad, f32 sweepRad,
                                f32 targetSpeed, f32 accel, f32 decel)
    {
      assert(radius >= 0);
      
      type_ = PST_ARC;
      def_.arc.centerPt_x = x_center;
      def_.arc.centerPt_y = y_center;
      def_.arc.radius = radius;
      def_.arc.startRad = startRad;
      def_.arc.sweepRad = sweepRad;

      // Compute end angle
      f32 radiusAngle = startRad + sweepRad;
      if (sweepRad > 0) {
        // Turning CCW so add 90 degree
        radiusAngle += M_PI_2_F;
      } else {
        radiusAngle -= M_PI_2_F;
      }
      
      // If going backwards, flip it 180
      def_.arc.endAngle = Radians(radiusAngle + (targetSpeed < 0 ? M_PI_F : 0)).ToFloat();
      
      SetSpeedProfile(targetSpeed, accel, decel);
    }
    
    void PathSegment::DefinePointTurn(f32 x, f32 y, f32 startAngle, f32 targetAngle,
                                      f32 targetRotSpeed, f32 rotAccel, f32 rotDecel,
                                      f32 angleTolerance,
                                      bool useShortestDir)
    {
      type_ = PST_POINT_TURN;
      def_.turn.x = x;
      def_.turn.y = y;
      def_.turn.startAngle = startAngle;
      def_.turn.targetAngle = targetAngle;
      def_.turn.angleTolerance = angleTolerance;
      def_.turn.useShortestDir = useShortestDir;
      
      SetSpeedProfile(targetRotSpeed, rotAccel, rotDecel);
    }

    void PathSegment::SetSpeedProfile(f32 targetSpeed, f32 accel, f32 decel)
    {
      targetSpeed_ = targetSpeed;
      accel_ = accel;
      decel_ = decel;
    }
    
    f32 PathSegment::GetLength() const
    {
      switch(type_)
      {
        case PST_LINE:
          return sqrtf((def_.line.startPt_x - def_.line.endPt_x)*(def_.line.startPt_x - def_.line.endPt_x) + (def_.line.startPt_y - def_.line.endPt_y)*(def_.line.startPt_y - def_.line.endPt_y));
        case PST_ARC:
          return ABS(def_.arc.sweepRad) * def_.arc.radius;
        case PST_POINT_TURN:
          return 0;
        default:
          CoreTechPrint("ERROR (Path::GetLength): Undefined segment %d\n", type_);
          assert(false);
      }
      return 0;
    }
    
    void PathSegment::OffsetStart(f32 xOffset, f32 yOffset)
    {
      switch(type_) {
      case PST_LINE:
        def_.line.startPt_x += xOffset;
        def_.line.startPt_y += yOffset;
        def_.line.endPt_x += xOffset;
        def_.line.endPt_y += yOffset;
        break;
      case PST_ARC:
        def_.arc.centerPt_x += xOffset;
        def_.arc.centerPt_y += yOffset;
        break;
      case PST_POINT_TURN:
        def_.turn.x += xOffset;
        def_.turn.y += yOffset;
        break;
      default:
        CoreTechPrint("ERROR (OffsetStart): Undefined segment %d\n", type_);
        assert(false);
      }
    }

    f32 PathSegment::GetStartAngle() const
    {
      switch(type_){
        case PST_LINE:
          return def_.line.endAngle;    // for straight lines, startAngle == endAngle
          break;
        case PST_ARC:
          return def_.arc.endAngle - def_.arc.sweepRad;
          break;
        case PST_POINT_TURN:
          return def_.turn.startAngle;
          break;
        default:
          CoreTechPrint("ERROR (GetStartAngle): Undefined segment %d\n", type_);
          assert(false);
          return 0;
      }
    }

    void PathSegment::GetStartPoint(f32 &x, f32 &y) const
    {
      switch(type_){
        case PST_LINE:
          x = def_.line.startPt_x;
          y = def_.line.startPt_y;
          break;
        case PST_ARC:
          x = def_.arc.centerPt_x + def_.arc.radius * cosf(def_.arc.startRad);
          y = def_.arc.centerPt_y + def_.arc.radius * sinf(def_.arc.startRad);
          break;
        case PST_POINT_TURN:
          x = def_.turn.x;
          y = def_.turn.y;
          break;
        default:
          CoreTechPrint("ERROR (GetStartPoint): Undefined segment %d\n", type_);
          assert(false);
      }
    }
    
    void PathSegment::GetEndPose(f32 &x, f32 &y, f32 &angle) const
    {
      switch(type_){
        case PST_LINE:
          x = def_.line.endPt_x;
          y = def_.line.endPt_y;
          angle = def_.line.endAngle;
          break;
        case PST_ARC:
          x = def_.arc.centerPt_x + def_.arc.radius * cosf(def_.arc.startRad + def_.arc.sweepRad);
          y = def_.arc.centerPt_y + def_.arc.radius * sinf(def_.arc.startRad + def_.arc.sweepRad);
          angle = def_.arc.endAngle;
          break;
        case PST_POINT_TURN:
          x = def_.turn.x;
          y = def_.turn.y;
          angle = def_.turn.targetAngle;
          break;
        default:
          CoreTechPrint("ERROR (GetEndPose): Undefined segment %d\n", type_);
          assert(false);
      }
    }

    
    
    void PathSegment::Print() const
    {
      switch(type_) {
        case PST_LINE:
        {
          const PathSegmentDef::s_line& seg = def_.line;
          CoreTechPrint("line: (%f, %f) to (%f, %f), speed/accel/decel = (%f, %f, %f)\n",
                 seg.startPt_x,
                 seg.startPt_y,
                 seg.endPt_x,
                 seg.endPt_y,
                 GetTargetSpeed(),
                 GetAccel(),
                 GetDecel());
          break;
        }
        case PST_ARC:
        {
          const PathSegmentDef::s_arc& seg = def_.arc;
          CoreTechPrint("arc: centerPt (%f, %f), radius %f, startAng %f, sweep %f, speed/accel/decel = (%f, %f, %f)\n",
                 seg.centerPt_x,
                 seg.centerPt_y,
                 seg.radius,
                 seg.startRad,
                 seg.sweepRad,
                 GetTargetSpeed(),
                 GetAccel(),
                 GetDecel());
          break;
        }
        case PST_POINT_TURN:
        {
          const PathSegmentDef::s_turn& seg = def_.turn;
          CoreTechPrint("ptTurn: x %f, y %f, targetAngle %f, tol %fdeg speed/accel/decel = (%f, %f, %f)\n",
                 seg.x,
                 seg.y,
                 seg.targetAngle,
                 RAD_TO_DEG( seg.angleTolerance ),
                 GetTargetSpeed(),
                 GetAccel(),
                 GetDecel());
          break;
        }
        default:
          break;
      }

    }

    
    SegmentRangeStatus PathSegment::GetDistToSegment(const f32 x, const f32 y, const f32 angle,
                                                     f32 &shortestDistanceToPath, f32 &radDiff,
                                                     f32 *distAlongSegmentFromClosestPointToEnd) const
    {
      SegmentRangeStatus res = OOR_NEAR_END;
      
      switch(type_) {
        case PST_LINE:
          res = GetDistToLineSegment(x,y,angle,shortestDistanceToPath,radDiff,
                                     distAlongSegmentFromClosestPointToEnd);
          break;
        case PST_ARC:
          res = GetDistToArcSegment(x,y,angle,shortestDistanceToPath,radDiff,
                                    distAlongSegmentFromClosestPointToEnd);
          break;
        case PST_POINT_TURN:
          // NOTE: This always returns IN_SEGMENT_RANGE since we can't know purely
          // from the given pose whether it's approaching or past the target angle.
          res = GetDistToPointTurnSegment(x,y,angle,shortestDistanceToPath,radDiff);
          break;
        default:
          assert(false);
      }
      
      return res;
    }
    

    
    SegmentRangeStatus PathSegment::GetDistToLineSegment(const f32 x, const f32 y, const f32 angle,
                                                         f32 &shortestDistanceToPath, f32 &radDiff,
                                                         f32 *distAlongSegmentFromClosestPointToEnd) const
    {
      SegmentRangeStatus segStatus = IN_SEGMENT_RANGE;
      f32 distToEnd = 0;
      
      const PathSegmentDef::s_line* seg = &(def_.line);
      
      f32 line_m_ = (seg->endPt_y - seg->startPt_y) / (seg->endPt_x - seg->startPt_x);
      f32 line_b_ = seg->startPt_y - line_m_ * seg->startPt_x;
      f32 line_dy_sign_ = ((seg->endPt_y - seg->startPt_y) >= 0) ? 1.0 : -1.0;
      Radians line_theta_ = ATAN2_FAST(seg->endPt_y - seg->startPt_y, seg->endPt_x - seg->startPt_x);
      
      
      // Find shortest path to current segment.
      // Shortest path is along a line with inverse negative slope (i.e. -1/m).
      // Point of intersection is solution to mx + b == (-1/m)*x + b_inv where b_inv = y-(-1/m)*x
      
#if(DEBUG_PATH)
      CoreTechPrint("LINE (%f, %f, %f, %f)\n", seg->startPt_x, seg->startPt_y, seg->endPt_x, seg->endPt_y);
      CoreTechPrint("Robot Pose: x: %f, y: %f ang: %f\n", x,y,angle);
#endif
      
      
      // Distance to start point
      f32 sqDistToStartPt = (seg->startPt_x - x) * (seg->startPt_x - x) +
      (seg->startPt_y - y) * (seg->startPt_y - y);
      
      // Distance to end point
      f32 sqDistToEndPt = (seg->endPt_x - x) * (seg->endPt_x - x) +
      (seg->endPt_y - y) * (seg->endPt_y - y);
      
      
      if (ABS(line_m_) > 10000) {
        // Special case: Vertical line
        if (seg->endPt_y > seg->startPt_y) {
          shortestDistanceToPath = seg->startPt_x - x;
        } else {
          shortestDistanceToPath = x - seg->startPt_x;
        }
        
        // Compute angle difference
        radDiff = (line_theta_ - angle).ToFloat();
        
        // Compute distance to end of segment
        distToEnd = ABS(seg->endPt_y - y);
        
        
        // If the point (x_intersect,y_intersect) is not between startPt and endPt,
        // and the robot is closer to the end point than it is to the start point,
        // then we've passed this segment and should go to next one
        if ( signbit(seg->startPt_y - y) == signbit(seg->endPt_y - y)) {
          if (sqDistToStartPt > sqDistToEndPt) {
            distToEnd = -distToEnd;
            segStatus = OOR_NEAR_END;
          } else {
            segStatus = OOR_NEAR_START;
          }
        }
        
      } else if (NEAR(line_m_, 0.f, 0.001f)) {
        // Special case: Horizontal line
        if (seg->endPt_x > seg->startPt_x) {
          shortestDistanceToPath = y - seg->startPt_y;
        } else {
          shortestDistanceToPath = seg->startPt_y - y;
        }
        
        // Compute angle difference
        radDiff = (line_theta_ - angle).ToFloat();
        
        // Compute distance to end of segment
        distToEnd = ABS(seg->endPt_x - x);
        
        // If the point (x_intersect,y_intersect) is not between startPt and endPt,
        // and the robot is closer to the end point than it is to the start point,
        // then we've passed this segment and should go to next one
        if ( signbit(seg->startPt_x - x) == signbit(seg->endPt_x - x)) {
          if (sqDistToStartPt > sqDistToEndPt) {
            distToEnd = -distToEnd;
            segStatus = OOR_NEAR_END;
          } else {
            segStatus = OOR_NEAR_START;
          }
        }
        
      } else {
        
        f32 b_inv = y + x/line_m_;
        
        f32 x_intersect = line_m_ * (b_inv - line_b_) / (line_m_*line_m_ + 1);
        f32 y_intersect = - (x_intersect / line_m_) + b_inv;
        
        f32 dy = y - y_intersect;
        f32 dx = x - x_intersect;
        
        shortestDistanceToPath = sqrtf(dy*dy + dx*dx);
        
#if(DEBUG_PATH)
        CoreTechPrint("m: %f, b: %f\n",line_m_,line_b_);
        CoreTechPrint("x_int: %f, y_int: %f, b_inv: %f\n", x_intersect, y_intersect, b_inv);
        CoreTechPrint("dy: %f, dx: %f, dist: %f\n", dy, dx, shortestDistanceToPath);
        CoreTechPrint("signbit(dx): %d, dy_sign: %f\n", signbit(dx), line_dy_sign_);
        CoreTechPrint("lineTheta: %f\n", line_theta_.ToFloat());
        //PRINT("lineTheta: %f, robotTheta: %f\n", seg->theta.ToFloat(), currPose.GetAngle().ToFloat());
#endif
        
        // Compute the sign of the error distance
        shortestDistanceToPath *= (signbit(line_m_) ? -1 : 1) * (signbit(dy) ? -1 : 1) * line_dy_sign_;
        
        
        // Compute angle difference
        radDiff = (line_theta_ - angle).ToFloat();
        
        // Compute distance to end of segment
        f32 dxToEnd = seg->endPt_x - x_intersect;
        f32 dyToEnd = seg->endPt_y - y_intersect;
        distToEnd = ABS( sqrtf(dxToEnd * dxToEnd + dyToEnd * dyToEnd) );
        
        // Did we pass the current segment?
        // If the point (x_intersect,y_intersect) is not between startPt and endPt,
        // and the robot is closer to the end point than it is to the start point,
        // then we've passed this segment and should go to next one
        if ( (signbit(seg->startPt_x - x_intersect) == signbit(seg->endPt_x - x_intersect))
            && (signbit(seg->startPt_y - y_intersect) == signbit(seg->endPt_y - y_intersect)) ) {
          if (sqDistToStartPt > sqDistToEndPt){
            distToEnd = -distToEnd;
            segStatus = OOR_NEAR_END;
          } else {
            segStatus = OOR_NEAR_START;
          }
        }
      }
      
      if (distAlongSegmentFromClosestPointToEnd) {
        *distAlongSegmentFromClosestPointToEnd = distToEnd;
      }
      
      return segStatus;
    }
    


    SegmentRangeStatus PathSegment::GetDistToArcSegment(const f32 x, const f32 y, const f32 angle,
                                                        f32 &shortestDistanceToPath, f32 &radDiff,
                                                        f32 *distAlongSegmentFromClosestPointToEnd) const
    {
      const PathSegmentDef::s_arc* seg = &(def_.arc);
      
#if(DEBUG_PATH)
      CoreTechPrint("ARC (%f, %f), startRad: %f, sweepRad: %f, radius: %f\n",
            seg->centerPt_x, seg->centerPt_y, seg->startRad, seg->sweepRad, seg->radius);
#endif
      
      
      // Assuming arc is broken up so that it is a true function
      
      // Arc paramters
      f32 x_center = seg->centerPt_x;
      f32 y_center = seg->centerPt_y;
      f32 r = seg->radius;
      Anki::Radians startRad = seg->startRad;
      
      // Line formed by arc center and robot pose
      f32 dy = y - y_center;
      f32 dx = x - x_center;
      
      // Find error between current robot heading and the expected heading at the closest point on the arc
      bool movingCCW = seg->sweepRad >= 0;
      Anki::Radians theta_line = ATAN2_FAST(dy,dx); // angle of line from circle center to robot
      Anki::Radians theta_tangent = theta_line + Anki::Radians((movingCCW ? 1 : -1 ) * M_PI_2_F);
      
      radDiff = (theta_tangent - angle).ToFloat();
      
      // Compute shortest distance to arc
      shortestDistanceToPath = sqrtf((x-x_center)*(x-x_center) + (y-y_center)*(y-y_center)) - r;
      
      // Compute sign of shortest distance
      // TODO: This method assumes that the robot is already mostly aligned with `theta_tangent`.
      //       Makes more sense to make the sign depend on `angle` as well.
      if (movingCCW) {
        shortestDistanceToPath *= -1;
      }

      // Compute distance to end of segment
      f32 distToEnd = ABS((Radians(seg->startRad + seg->sweepRad) - theta_line).ToFloat() * seg->radius);

      
      // Did we pass the current segment?
      // Check if the angDiff exceeds the sweep angle.
      // Also check for transitions between -PI and +PI by checking if angDiff
      // ever exceeds a conservative half the distance if PI was approached from the opposite direction.
      SegmentRangeStatus segStatus = IN_SEGMENT_RANGE;
      f32 angDiff = (theta_line - startRad).ToFloat();
      
      if (movingCCW) {
        if (angDiff > seg->sweepRad || angDiff < -0.5f*(2.f*M_PI_F-seg->sweepRad)) {
          distToEnd = -distToEnd;
          segStatus = OOR_NEAR_END;
        } else if (angDiff < 0 && angDiff > -0.5f*(2.f*M_PI_F-seg->sweepRad)) {
          segStatus = OOR_NEAR_START;
        }
          
      } else {
        if (angDiff < seg->sweepRad || angDiff >  0.5f*(2.f*M_PI_F+seg->sweepRad)) {
          distToEnd = -distToEnd;
          segStatus = OOR_NEAR_END;
        } else if (angDiff > 0 && angDiff < 0.5f*(2.f*M_PI_F-seg->sweepRad)) {
          segStatus = OOR_NEAR_START;
        }
      }
      

      if (distAlongSegmentFromClosestPointToEnd) {
        *distAlongSegmentFromClosestPointToEnd = distToEnd;
      }
      
#if(DEBUG_PATH)
      CoreTechPrint("x: %f, y: %f, m: %f, b: %f\n", x,y,m,b);
      CoreTechPrint("x_center: %f, y_center: %f\n", x_center, y_center);
      CoreTechPrint("x_int: %f, y_int: %f\n", x_intersect, y_intersect);
      CoreTechPrint("dy: %f, dx: %f, dist: %f, radDiff: %f\n", dy, dx, shortestDistanceToPath, radDiff);
      CoreTechPrint("insideCircle: %d, segmentRangeStatus: %d\n", robotInsideCircle, segStatus);
      CoreTechPrint("theta_line: %f, theta_tangent: %f\n", theta_line.ToFloat(), theta_tangent.ToFloat());
#endif
      
      return segStatus;
    }
    
    

    SegmentRangeStatus PathSegment::GetDistToPointTurnSegment(const f32 x, const f32 y, const f32 angle,
                                                        f32 &shortestDistanceToPath, f32 &radDiff) const
    {
      const PathSegmentDef::s_turn* seg = &(def_.turn);
      
#if(DEBUG_PATH)
      CoreTechPrint("TURN (%f, %f), targetAngle: %f, targetRotSpeed: %f\n",
            seg->x, seg->y, seg->targetAngle, GetTargetSpeed());
#endif
      
      shortestDistanceToPath = sqrtf( (x - seg->x)*(x - seg->x) + (y - seg->y)*(y - seg->y) );
      Radians currAngle(angle);
      Radians targetAngle(seg->targetAngle);
      radDiff = currAngle.angularDistance(targetAngle,GetTargetSpeed() < 0);
      
      return IN_SEGMENT_RANGE;
    }
    
    
    /////////////// Path implementations ////////////////

    Path::Path()
    {
      capacity_ = MAX_NUM_PATH_SEGMENTS;

#ifdef CORETECH_ROBOT
  #if defined CORETECH_ENGINE
  #error "only one of CORETECH_ENGINE or CORETECH_ROBOT can be defined"
  #endif
      path_ = __pathSegmentStackForRobot;
#elif defined CORETECH_ENGINE
      path_ = new PathSegment[MAX_NUM_PATH_SEGMENTS];
#else
#error "one of CORETECH_ENGINE or CORETECH_ROBOT must be defined"
#endif

      Clear();
    }

    Path::Path(const Path& other)
    {
      capacity_ = MAX_NUM_PATH_SEGMENTS;

#ifdef CORETECH_ROBOT
  #if defined CORETECH_ENGINE
  #error "only one of CORETECH_ENGINE or CORETECH_ROBOT can be defined"
  #endif
      path_ = __pathSegmentStackForRobot;
#elif defined CORETECH_ENGINE
      path_ = new PathSegment[MAX_NUM_PATH_SEGMENTS];
#else
#error "one of CORETECH_ENGINE or CORETECH_ROBOT must be defined"
#endif

      Clear();

      *this = other;
    }

    Path::~Path()
    {
#ifdef CORETECH_ENGINE
      delete [] path_;
      path_ = nullptr;
#endif
    }
    
    Path& Path::operator=(const Path& rhs)
    {
      capacity_ = MAX_NUM_PATH_SEGMENTS;
      assert(capacity_ == rhs.capacity_);

      numPathSegments_ = rhs.numPathSegments_;
      memcpy(path_, rhs.path_, numPathSegments_*sizeof(PathSegment));
      return *this;
    }

    void Path::Clear()
    {
      assert(capacity_ == MAX_NUM_PATH_SEGMENTS);

      numPathSegments_ = 0;
    }

    
    void Path::PrintPath() const
    {
      assert(capacity_ == MAX_NUM_PATH_SEGMENTS);

      for(u8 i = 0; i<numPathSegments_; ++i) {
        PrintSegment(i);
      }
    }
    
    void Path::PrintSegment(u8 segment) const
    {
      assert(capacity_ == MAX_NUM_PATH_SEGMENTS);

      if (segment < numPathSegments_) {
        CoreTechPrint("Path segment %d - ", segment);
        path_[segment].Print();
      }
    }
      
    // Returns angle between two points on a circle
    f32 GetArcAngle(f32 start_x, f32 start_y, f32 end_x, f32 end_y, f32 center_x, f32 center_y, bool CCW)
    {
      f32 a_start_x, a_start_y;  // Vector from circle center to starting point on circle when computing arc length
      f32 a_end_x, a_end_y;      // Vector from circle center to end point on circle when computing arc length
      
      
      a_start_x = start_x - center_x;
      a_start_y = start_y - center_y;
      
      a_end_x = end_x - center_x;
      a_end_y = end_y - center_y;
      
      f32 theta = ATAN2_ACC(a_end_y, a_end_x) - ATAN2_ACC(a_start_y, a_start_x);
      if (theta < 0 && CCW)
        return theta + 2*M_PI_F;
      else if (theta > 0 && !CCW)
        return theta - 2*M_PI_F;
      
      return theta;
    }
    
    
    // Generates a CSC Dubins curve if one exists.
    // Returns the number of segments in the path.
    u8 GenerateCSCCurve(f32 startPt_x, f32 startPt_y, f32 startPt_theta,
                        f32 endPt_x, f32 endPt_y, f32 endPt_theta,
                        f32 start_radius, f32 end_radius,
                        f32 targetSpeed, f32 accel, f32 decel,
                        DubinsPathType pathType, PathSegment path[], f32 &path_length)
    {
     
      assert(pathType != NUM_DUBINS_PATHS);
      
      // Compute LSL, LSR, RSR, RSL paths
      // http://gieseanw.wordpress.com/2012/10/21/a-comprehensive-step-by-step-tutorial-to-computing-dubins-paths/
      
      // p_c1: Center of circle 1 (for start pose) which has radius r1.
      // p_c2: Center of circle 2 (for end pose) which has radius r2
      // V1: Vector from p_c1 to p_c2
      // V2: Tangent vector from p_t1 on circle 1 to p_t2 on circle 2
      // n: Unit normal vector (perpendicular to V2)
      //
      // V1 can be made to be parallel to V2 by subtracting (r1-r2) • n
      // So n • (V1 - ((r1-r2) • n)) = 0, since n and (V1 - ((r1-r2) • n)) are orthogonal
      //    n • V1 + r2 - r1 = 0
      //    n • V1 = r1 - r2
      //
      // Let D be the magnitude of V1.
      // Let v1 be V1 normalized so that
      //
      //    n • v1 = (r1 - r2)/D
      //
      // Let c = (r1-r2)/D
      // c is the cosine of the angle between v1 and n, since A • B == |A| |B| cos(theta).
      //
      // This means the sine of the angle is sqrt(1-c^2), since sin^2 + cos^2 = 1.
      //
      // If v1 is rotated by acos(c) then we have n.
      //
      // n_x = v1_x * c - v1_y * sqrt(1-c^2)
      // n_y = v1_x * sqrt(1-c^2) + v1_y * c
      //
      // Follow n by r1 from p_c1 to get p_t1.
      // Follow n by r2 from p_c2 to get p_t2.
      
      
      f32 r1 = ABS(start_radius);
      f32 r2 = ABS(end_radius);
      f32 p_c1_x, p_c1_y;    // Center point of circle 1
      f32 p_c2_x, p_c2_y;    // Center point of circle 2
      f32 p_t1_x, p_t1_y;    // Tangent point on circle 1
      f32 p_t2_x, p_t2_y;    // Tangent point on circle 2
      f32 V1_x, V1_y;        // Vector from p_c1 to p_t1
      f32 n_x, n_y;          // Orthogonal vector to tangent line
      f32 v1_x, v1_y;        // Unit vector of V1
      f32 V1_mag;            // Magnitude of V1
      f32 cosTanPtAngle, sinTanPtAngle;
      f32 segment_length;
      u8 num_segments = 0;
      PathSegment* ps;
      
      path_length = FLT_MAX;

      
      f32 sign1;
      f32 sign2;
      f32 minCircleDist;
      switch(pathType) {
        case RSR:
          sign1 = 1.0;
          sign2 = 1.0;
          minCircleDist = ABS(r1 - r2);
          break;
        case LSL:
          sign1 = -1.0;
          sign2 = -1.0;
          minCircleDist = ABS(r1 - r2);
          break;
        case RSL:
          sign1 = 1.0;
          sign2 = -1.0;
          minCircleDist = r1 + r2;
          break;
        case LSR:
          sign1 = -1.0;
          sign2 = 1.0;
          minCircleDist = r1 + r2;
          break;
        default:
          return 0;
      }
      
      
      // Compute center of circle 1
      p_c1_x = startPt_x + sign1 * r1 * sinf(startPt_theta);
      p_c1_y = startPt_y - sign1 * r1 * cosf(startPt_theta);
      
      // Compute center of circle 2
      p_c2_x = endPt_x + sign2 * r2 * sinf(endPt_theta);
      p_c2_y = endPt_y - sign2 * r2 * cosf(endPt_theta);
      
      // Compute V1
      V1_x = p_c2_x - p_c1_x;
      V1_y = p_c2_y - p_c1_y;
      V1_mag = sqrtf(V1_x * V1_x + V1_y * V1_y);

      // Check if circle centers are too close together
      if (V1_mag <= minCircleDist) {
        return 0;
      }
      
      v1_x = V1_x / V1_mag;
      v1_y = V1_y / V1_mag;
      
      // Compute cosTanPtAngle (aka c) and sinTanPtAngle
      cosTanPtAngle = (sign1 * r1 - sign2 * r2) / V1_mag;
      sinTanPtAngle = sqrtf(1-cosTanPtAngle*cosTanPtAngle);
      
      // Compute n
      n_x = v1_x * cosTanPtAngle - v1_y * sinTanPtAngle;
      n_y = v1_x * sinTanPtAngle + v1_y * cosTanPtAngle;
      
      // Compute tangent points
      p_t1_x = p_c1_x + n_x * r1 * sign1;
      p_t1_y = p_c1_y + n_y * r1 * sign1;
      
      p_t2_x = p_c2_x + n_x * r2 * sign2;
      p_t2_y = p_c2_y + n_y * r2 * sign2;
      
      
#if(DEBUG_PATH)
       f32 tanPtAngle = ATAN2_ACC(n_y, n_x);
       CoreTechPrint("Dubins %d: \n"
       " p_c1 (%f, %f)\n"
       " p_c2 (%f, %f)\n"
       " V1 (%f %f)\n"
       " n (%f %f)\n"
       " p_t1 (%f, %f)\n"
       " p_t2 (%f, %f)\n",
       pathType,
       p_c1_x, p_c1_y,
       p_c2_x, p_c2_y,
       V1_x, V1_y,
       n_x, n_y,
       p_t1_x, p_t1_y,
       p_t2_x, p_t2_y
       );
#endif
      

      // Generate path segment definitions
      path_length = 0;
      ps = &path[num_segments];
      ps->DefineArc(p_c1_x, p_c1_y,
                    start_radius,
                    ATAN2_ACC(startPt_y - p_c1_y, startPt_x - p_c1_x),
                    GetArcAngle(startPt_x, startPt_y, p_t1_x, p_t1_y, p_c1_x, p_c1_y, sign1 < 0),
                    targetSpeed, accel, decel);
      segment_length = ps->GetLength();
      
      if(segment_length > 0) {
        path_length += segment_length;
        ++num_segments;
      }
      
      ps = &path[num_segments];
      ps->DefineLine(p_t1_x, p_t1_y, p_t2_x, p_t2_y,
                      targetSpeed, accel, decel);
      segment_length = ps->GetLength();

      if(segment_length > 0) {
        path_length += segment_length;
        ++num_segments;
      }
      
      ps = &path[num_segments];
      ps->DefineArc(p_c2_x, p_c2_y,
                    end_radius,
                    ATAN2_ACC(p_t2_y - p_c2_y, p_t2_x - p_c2_x),
                    GetArcAngle(p_t2_x, p_t2_y, endPt_x, endPt_y, p_c2_x, p_c2_y, sign2 < 0),
                    targetSpeed, accel, decel);
      segment_length = ps->GetLength();

      if(segment_length > 0) {
        path_length += segment_length;
        ++num_segments;
      }
      
      return num_segments;
    }
    
  
    // Generates the Dubins path between a start and end pose with a constraint
    // on the minimum radius of the curved sections.
    // Returns the number of segments in the path, which should be 3.
    // Returns 0 if fails.
    //
    // NOTE: This is a very simple version which only works when
    // the difference between start_theta and end_theta is < pi/2
    // and also when the end pose is in front of the start pose.
    // Fails automatically
    u8 GenerateDubinsPath(Path &path,
                          f32 start_x, f32 start_y, f32 start_theta,
                          f32 end_x, f32 end_y, f32 end_theta,
                          f32 start_radius, f32 end_radius,
                          f32 targetSpeed, f32 accel, f32 decel,
                          f32 final_straight_approach_length,
                          f32 *path_length)
    {
      PathSegment csc_path[NUM_DUBINS_PATHS][3];
      
      u32 shortestNumSegments = 0;
      f32 shortestPathLength = FLT_MAX;
      DubinsPathType shortestPathType = NUM_DUBINS_PATHS;
      u32 numSegments;
      f32 pathLength;
      
      
      // Compute end point before the final straight segment
      // and then append the straight segment after the Dubins path is generated
      f32 preStraightApproach_x = end_x - final_straight_approach_length * cosf(end_theta);
      f32 preStraightApproach_y = end_y - final_straight_approach_length * sinf(end_theta);
      
      
#if(DEBUG_PATH)
      CoreTechPrint("DUBINS: startPt %f %f %f, preEnd %f %f, endPt %f %f %f, start_radius %f, end_radius %f\n",
            start_x, start_y, start_theta, preStraightApproach_x, preStraightApproach_y, end_x, end_y, end_theta, start_radius, end_radius);
#endif

      for (DubinsPathType i = LSL; i != NUM_DUBINS_PATHS; i = (DubinsPathType)(i+1)) {

        numSegments = GenerateCSCCurve(start_x, start_y, start_theta, preStraightApproach_x, preStraightApproach_y, end_theta, start_radius, end_radius,  targetSpeed, accel, decel, i, csc_path[i], pathLength);
#if(DEBUG_PATH)
        CoreTechPrint("Dubins path %d: numSegments %d, length %f m\n", i, numSegments, pathLength);
#endif
        if (pathLength < shortestPathLength) {
          shortestNumSegments = numSegments;
          shortestPathLength = pathLength;
          shortestPathType = i;
        }
      }

      // If a path was found, copy it to the path_ and tack on the final straight segment.
      if (shortestPathType != NUM_DUBINS_PATHS) {
        
        //path.numPathSegments_ = shortestNumSegments;
        for (u32 j = 0; j < shortestNumSegments; ++j) {
          //path_[j] = csc_path[shortestPathType][j];
          
          switch(csc_path[shortestPathType][j].GetType()) {
            case PST_LINE:
            {
              const PathSegmentDef::s_line *l = &(csc_path[shortestPathType][j].GetDef().line);
              path.AppendLine(l->startPt_x, l->startPt_y, l->endPt_x, l->endPt_y, targetSpeed, accel, decel);
              break;
            }
            case PST_ARC:
            {
              const PathSegmentDef::s_arc *a = &(csc_path[shortestPathType][j].GetDef().arc);
              path.AppendArc(a->centerPt_x, a->centerPt_y, a->radius, a->startRad, a->sweepRad, targetSpeed, accel, decel);
              break;
            }
            default:
              CoreTechPrint("ERROR: Invalid path segment type in Dubins path. Should not be possible!\n");
              assert(0);
              break;
          }
          
        }
        
        // Optionally, append a 4th straight line segment
        if (final_straight_approach_length != 0) {
          path.AppendLine(preStraightApproach_x, preStraightApproach_y, end_x, end_y, targetSpeed, accel, decel);
          shortestPathLength += final_straight_approach_length;
        }
        
        if (path_length) {
          *path_length = shortestPathLength;
        }
      }
      
#if(DEBUG_PATH)
      CoreTechPrint("Dubins: Shortest path %d, length %f\n", shortestPathType, shortestPathLength);
#endif
      
      return path.GetNumSegments();
    }
  
    bool Path::PopFront(const u8 numSegments)
    {
      assert(capacity_ == MAX_NUM_PATH_SEGMENTS);

      if (numSegments <= numPathSegments_) {
        numPathSegments_ -= numSegments;
        
        // Shift path segments to front down
        memcpy(path_, &(path_[numSegments]), numPathSegments_*sizeof(PathSegment));
        
        return true;
      } else {
        #if(DEBUG_PATH)
        CoreTechPrint("WARNING(Path::PopFront): Can't pop %d segments from %d segment path\n", numSegments, numPathSegments_);
        #endif
      }
      
      return false;
    }
    
    bool Path::PopBack(const u8 numSegments)
    {
      assert(capacity_ == MAX_NUM_PATH_SEGMENTS);

      if (numSegments <= numPathSegments_) {
        numPathSegments_ -= numSegments;
        return true;
      } else {
#if(DEBUG_PATH)
        CoreTechPrint("WARNING(Path::PopBack): Can't pop %d segments from %d segment path\n", numSegments, numPathSegments_);
#endif
      }
      
      return false;
    }
    
    

    // TODO: Eventually, this function should also be made to check that
    // orientation is not discontiuous and that velocity profile is smooth
    // at transition points.
    bool Path::CheckSegmentContinuity(f32 tolerance_distance_squared, s8 pathSegmentIdx) const
    {
      assert(capacity_ == MAX_NUM_PATH_SEGMENTS);

      // If checking continuity on non-existent piece
      if (pathSegmentIdx >= numPathSegments_)
        return false;
      
      // If checking continuity on first piece
      if (pathSegmentIdx == 0)
        return true;
    
      // Compute distance between start point of specified segment to end point of previous segment
      f32 start_x, start_y, end_x, end_y, end_angle;
      path_[pathSegmentIdx].GetStartPoint(start_x, start_y);
      path_[pathSegmentIdx-1].GetEndPose(end_x, end_y, end_angle);
      if ((start_x - end_x)*(start_x - end_x) + (start_y - end_y)*(start_y - end_y) < tolerance_distance_squared) {
        return true;
      }
  
      CoreTechPrint("Continuity fail: Segment %d start point (%f, %f), Segment %d end point (%f, %f)\n",
            pathSegmentIdx, start_x, start_y, pathSegmentIdx - 1, end_x, end_y);
      return false;
    }
  
    bool Path::CheckContinuity(f32 tolerance_distance_squared, s8 pathSegmentIdx) const
    {
      assert(capacity_ == MAX_NUM_PATH_SEGMENTS);

      // Check entire path?
      if (pathSegmentIdx < 0) {
        for (u8 i=0; i< numPathSegments_; ++i) {
          if (!CheckSegmentContinuity(tolerance_distance_squared, i)) {
            CoreTechPrint("ERROR: Continuity check failed on segment %d of %d\n", i, GetNumSegments());
            return false;
          }
        }
        return true;
      }
      
      // Just check specified segment
      return CheckSegmentContinuity(tolerance_distance_squared, pathSegmentIdx);
    }

    // Add path segment
    // tODO: Change units to meters
    bool Path::AppendLine(f32 x_start, f32 y_start, f32 x_end, f32 y_end,
                          f32 targetSpeed, f32 accel, f32 decel)
    {
      assert(capacity_ == MAX_NUM_PATH_SEGMENTS);

      if (numPathSegments_ >= MAX_NUM_PATH_SEGMENTS) {
        CoreTechPrint("ERROR (AppendLine): Exceeded path size\n");
        return false;
      }
      
      path_[numPathSegments_].DefineLine(x_start, y_start, x_end, y_end,
                                         targetSpeed, accel, decel);

#if DEBUG_PATH_APPEND
      CoreTechPrint("INFO (AppendLine): numPathSegments_ = %u :", numPathSegments_);
      path_[numPathSegments_].Print();
#endif

      numPathSegments_++;
      
      return true;
    }
  
  
    void Path::AddArc(f32 x_center, f32 y_center, f32 radius, f32 startRad, f32 sweepRad,
                      f32 targetSpeed, f32 accel, f32 decel) {
      assert(capacity_ == MAX_NUM_PATH_SEGMENTS);

      if (FLT_NEAR(sweepRad,0.f)) {
        CoreTechPrint("ERROR: sweepRad is zero\n");
      }

      path_[numPathSegments_].DefineArc(x_center, y_center, radius, startRad, sweepRad,
                                        targetSpeed, accel, decel);

#if DEBUG_PATH_APPEND
      CoreTechPrint("INFO (AddArc): numPathSegments_ = %u :", numPathSegments_);
      path_[numPathSegments_].Print();
#endif

      numPathSegments_++;
    }
  
  
    bool Path::AppendArc(f32 x_center, f32 y_center, f32 radius, f32 startRad, f32 sweepRad,
                         f32 targetSpeed, f32 accel, f32 decel)
    {
      assert(capacity_ == MAX_NUM_PATH_SEGMENTS);

      if (numPathSegments_ >= MAX_NUM_PATH_SEGMENTS) {
        CoreTechPrint("ERROR (AppendArc): Exceeded path size\n");
        return false;
      }
      
      if (FLT_NEAR(sweepRad,0.f)) {
        CoreTechPrint("ERROR: sweepRad is zero\n");
        return false;
      }
      

      // Make sure startRad is between -PI and PI
      startRad = Radians(startRad).ToFloat();

      
      // Arcs need to be true functions.
      // i.e. for any value of x there can only be one y value.
      // This also means that an arc cannot sweep past the angles 0 or PI.
      //
      // Split up into several arcs if necessary.
      
      f32 sweepRadLeft = ABS(sweepRad);
      f32 sweep;
      Radians currAngle(startRad);
      Radians zeroAngle(0);
      Radians piAngle(M_PI_F);
      
      // limitAngle toggles between zeroAngle and piAngle for as
      // long as traversing sweepRad causes it to cross 0 or PI.
      Radians limitAngle = zeroAngle;

      if ((currAngle >= zeroAngle && currAngle != piAngle && sweepRad > 0) ||
          (currAngle < zeroAngle && sweepRad < 0)) {
        limitAngle = piAngle;
      }

      
      while(sweepRadLeft > 0) {
        
        if (sweepRad > 0) {
          // sweeping CCW
          sweep = MIN( ABS((limitAngle - currAngle).ToFloat()), sweepRadLeft);
        } else {
          // sweeping CW
          sweep = MAX( -ABS((limitAngle - currAngle).ToFloat()), -sweepRadLeft);
        }
        
        if(!NEAR_ZERO(sweep)) {
          AddArc(x_center, y_center, radius, currAngle.ToFloat(), sweep,
                 targetSpeed, accel, decel);
        }
          
        if (ABS(sweep) == sweepRadLeft) {
          sweepRadLeft = 0;
        } else {
          currAngle = limitAngle;
          sweepRadLeft -= ABS(sweep);
        }
        
        // toggle limit angle
        limitAngle = (limitAngle == piAngle ? zeroAngle : piAngle);

      }
      
      
      return true;
    }
    
    
    bool Path::AppendPointTurn(f32 x, f32 y, f32 startAngle, f32 targetAngle,
                               f32 targetRotSpeed, f32 rotAccel, f32 rotDecel,
                               f32 angleTolerance,
                               bool useShortestDir)
    {
      assert(capacity_ == MAX_NUM_PATH_SEGMENTS);

      if (numPathSegments_ >= MAX_NUM_PATH_SEGMENTS) {
        CoreTechPrint("ERROR (AppendPointTurn): Exceeded path size\n");
        return false;
      }
      
      path_[numPathSegments_].DefinePointTurn(x,y,startAngle, targetAngle,
                                              targetRotSpeed, rotAccel, rotDecel,
                                              angleTolerance,
                                              useShortestDir);

      numPathSegments_++;
      
      return true;
    }

    bool Path::AppendSegment(const PathSegment& segment)
    {
      assert(capacity_ == MAX_NUM_PATH_SEGMENTS);

      if (numPathSegments_ >= MAX_NUM_PATH_SEGMENTS) {
        CoreTechPrint("ERROR (AppendSegment): Exceeded path size\n");
        return false;
      }

      path_[numPathSegments_] = segment;

#if DEBUG_PATH_APPEND
      CoreTechPrint("INFO (AppendSegment): numPathSegments_ = %u :", numPathSegments_);
      path_[numPathSegments_].Print();
#endif

      numPathSegments_++;

      return true;
    }  
    

  } // namespace Planning
} // namespace Anki
