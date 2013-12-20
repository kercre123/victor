#include "anki/cozmo/robot/path.h"
#include "anki/cozmo/robot/debug.h"
#include "anki/common/robot/utilities_c.h"
#include "anki/common/robot/trig_fast.h"
#include "anki/cozmo/robot/hal.h"

#include <math.h>

#define CONTINUITY_TOL_M 0.0001


namespace Anki
{
  namespace Cozmo
  {

    
    ////////////// PathSegment implementations ////////////
    
    Anki::Embedded::Point2f PathSegment::GetStartPoint() const
    {
      Anki::Embedded::Point2f p;
      switch(type){
        case PST_LINE:
          p.x = def.line.startPt_x;
          p.y = def.line.startPt_y;
          break;
        case PST_ARC:
          p.x = def.arc.centerPt_x + def.arc.radius * cosf(def.arc.startRad);
          p.y = def.arc.centerPt_y + def.arc.radius * sinf(def.arc.startRad);
          break;
        case PST_POINT_TURN:
          p.x = def.turn.x;
          p.y = def.turn.y;
          break;
        default:
          PRINT("ERROR (GetStartPoint): Undefined segment %d\n", type);
          break;
      }
      
      return p;
    }
    
    Anki::Embedded::Point2f PathSegment::GetEndPoint() const
    {
      Anki::Embedded::Point2f p;
      switch(type){
        case PST_LINE:
          p.x = def.line.endPt_x;
          p.y = def.line.endPt_y;
          break;
        case PST_ARC:
          p.x = def.arc.centerPt_x + def.arc.radius * cosf(def.arc.startRad + def.arc.sweepRad);
          p.y = def.arc.centerPt_y + def.arc.radius * sinf(def.arc.startRad + def.arc.sweepRad);
          break;
        case PST_POINT_TURN:
          p.x = def.turn.x;
          p.y = def.turn.y;
          break;
        default:
          PRINT("ERROR (GetEndPoint): Undefined segment %d\n", type);
          break;
      }
      
      return p;
    }
    
    void PathSegment::Print() const
    {
      switch(type) {
        case PST_LINE:
          PRINT("line: (%f, %f) to (%f, %f)\n",
                def.line.startPt_x,
                def.line.startPt_y,
                def.line.endPt_x,
                def.line.endPt_y);
          break;
        case PST_ARC:
          PRINT("arc: centerPt (%f, %f), radius %f, startAng %f, sweep %f\n",
                def.arc.centerPt_x,
                def.arc.centerPt_y,
                def.arc.radius,
                def.arc.startRad,
                def.arc.sweepRad);
          break;
        case PST_POINT_TURN:
          PRINT("ptTurn: x %f, y %f, targetAngle %f, maxAngularVel %f, angularAccel %f, angularDecel %f\n",
                def.turn.x,
                def.turn.y,
                def.turn.targetAngle,
                def.turn.maxAngularVel,
                def.turn.angularAccel,
                def.turn.angularDecel);
          break;
        default:
          break;
      }

    }

    
    
    /////////////// Path implementations ////////////////

    Path::Path()
    {
      Clear();
    }
    
    void Path::Clear()
    {
      numPathSegments_ = 0;
    }
    
    int Path::GetNumSegments() const
    {
      return numPathSegments_;
    }

    
    void Path::PrintPath() const
    {
      for(u8 i = 0; i<numPathSegments_; ++i) {
        PrintSegment(i);
      }
    }
    
    void Path::PrintSegment(u8 segment) const
    {
      if (segment < numPathSegments_) {
        PRINT("Path segment %d - ", segment);
        path_[segment].Print();
      }
    }
      
  
    f32 Path::GetArcAngle(f32 start_x, f32 start_y, f32 end_x, f32 end_y, f32 center_x, f32 center_y, bool CCW)
    {
      f32 a_start_x, a_start_y;  // Vector from circle center to starting point on circle when computing arc length
      f32 a_end_x, a_end_y;      // Vector from circle center to end point on circle when computing arc length
      
      
      a_start_x = start_x - center_x;
      a_start_y = start_y - center_y;
      
      a_end_x = end_x - center_x;
      a_end_y = end_y - center_y;
      
      f32 theta = atan2_acc(a_end_y, a_end_x) - atan2_acc(a_start_y, a_start_x);
      if (theta < 0 && CCW)
        return theta + 2*PI_F;
      else if (theta > 0 && !CCW)
        return theta - 2*PI_F;
      
      return theta;
    }
    
    
    
    u8 Path::GenerateCSCCurve(f32 startPt_x, f32 startPt_y, f32 startPt_theta,
                               f32 endPt_x, f32 endPt_y, f32 endPt_theta,
                               f32 start_radius, f32 end_radius,
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
      f32 cosTanPtAngle, sinTanPtAngle, tanPtAngle;
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
      
      tanPtAngle = atan2_acc(n_y, n_x);
      
      // Compute tangent points
      p_t1_x = p_c1_x + n_x * r1 * sign1;
      p_t1_y = p_c1_y + n_y * r1 * sign1;
      
      p_t2_x = p_c2_x + n_x * r2 * sign2;
      p_t2_y = p_c2_y + n_y * r2 * sign2;
      
      
#if(DEBUG_PATH)
       PRINT("Dubins %d: \n"
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
      ps->type = PST_ARC;
      ps->def.arc.centerPt_x = p_c1_x;
      ps->def.arc.centerPt_y = p_c1_y;
      ps->def.arc.radius = start_radius;
      ps->def.arc.startRad = atan2_acc(startPt_y - p_c1_y, startPt_x - p_c1_x);
      ps->def.arc.sweepRad = GetArcAngle(startPt_x, startPt_y, p_t1_x, p_t1_y, p_c1_x, p_c1_y, sign1 < 0);
      segment_length = ABS(ps->def.arc.sweepRad) * start_radius;
      
      if(segment_length > 0) {
        path_length += segment_length;
        ++num_segments;
      }
      
      ps = &path[num_segments];
      ps->type = PST_LINE;
      ps->def.line.startPt_x = p_t1_x;
      ps->def.line.startPt_y = p_t1_y;
      ps->def.line.endPt_x = p_t2_x;
      ps->def.line.endPt_y = p_t2_y;
      segment_length = sqrtf((p_t2_x - p_t1_x)*(p_t2_x - p_t1_x) + (p_t2_y - p_t1_y)*(p_t2_y - p_t1_y));

      if(segment_length > 0) {
        path_length += segment_length;
        ++num_segments;
      }
      
      ps = &path[num_segments];
      ps->type = PST_ARC;
      ps->def.arc.centerPt_x = p_c2_x;
      ps->def.arc.centerPt_y = p_c2_y;
      ps->def.arc.radius = end_radius;
      ps->def.arc.startRad = atan2_acc(p_t2_y - p_c2_y, p_t2_x - p_c2_x);
      ps->def.arc.sweepRad = GetArcAngle(p_t2_x, p_t2_y, endPt_x, endPt_y, p_c2_x, p_c2_y, sign2 < 0);
      segment_length = ABS(ps->def.arc.sweepRad) * end_radius;
      
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
    u8 Path::GenerateDubinsPath(f32 start_x, f32 start_y, f32 start_theta,
                            f32 end_x, f32 end_y, f32 end_theta,
                            f32 start_radius, f32 end_radius,
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
      PRINT("DUBINS: startPt %f %f %f, preEnd %f %f, endPt %f %f %f, start_radius %f, end_radius %f\n",
            start_x, start_y, start_theta, preStraightApproach_x, preStraightApproach_y, end_x, end_y, end_theta, start_radius, end_radius);
#endif

      for (DubinsPathType i = LSL; i != NUM_DUBINS_PATHS; i = (DubinsPathType)(i+1)) {

        numSegments = GenerateCSCCurve(start_x, start_y, start_theta, preStraightApproach_x, preStraightApproach_y, end_theta, start_radius, end_radius, i, csc_path[i], pathLength);
#if(DEBUG_PATH)
        PRINT("Dubins path %d: numSegments %d, length %f m\n", i, numSegments, pathLength);
#endif
        if (pathLength < shortestPathLength) {
          shortestNumSegments = numSegments;
          shortestPathLength = pathLength;
          shortestPathType = i;
        }
      }

      // If a path was found, copy it to the path_ and tack on the final straight segment.
      if (shortestPathType != NUM_DUBINS_PATHS) {
        
        numPathSegments_ = shortestNumSegments;
        for (u32 j = 0; j < numPathSegments_; ++j) {
          path_[j] = csc_path[shortestPathType][j];
        }
        AppendLine(0, preStraightApproach_x, preStraightApproach_y, end_x, end_y);
        
        shortestPathLength += final_straight_approach_length;
        
        if (path_length) {
          *path_length = shortestPathLength;
        }
      }
      
#if(DEBUG_PATH)
      PRINT("Dubins: Shortest path %d, length %f\n", shortestPathType, shortestPathLength);
#endif
      
      return numPathSegments_;
    }
  

    // TODO: Eventually, this function should also be made to check that
    // orientation is not discontiuous and that velocity profile is smooth
    // at transition points.
    bool Path::CheckSegmentContinuity(u8 pathSegmentIdx) const
    {
      // If checking continuity on non-existent piece
      if (pathSegmentIdx >= numPathSegments_)
        return false;
      
      // If checking continuity on first piece
      if (pathSegmentIdx == 0)
        return true;
    
      // Compute distance between start point of specified segment to end point of previous segment
      if (path_[pathSegmentIdx].GetStartPoint().Dist(path_[pathSegmentIdx-1].GetEndPoint()) < CONTINUITY_TOL_M) {
        return true;
      }
  
      PRINT("Continuity fail: Segment %d start point (%f, %f), Segment %d end point (%f, %f)\n",
            pathSegmentIdx, path_[pathSegmentIdx].GetStartPoint().x, path_[pathSegmentIdx].GetStartPoint().y,
            pathSegmentIdx-1, path_[pathSegmentIdx-1].GetEndPoint().x, path_[pathSegmentIdx-1].GetEndPoint().y
            );
      return false;
    }
  
    bool Path::CheckContinuity(s16 pathSegmentIdx) const
    {
      // Check entire path?
      if (pathSegmentIdx < 0) {
        for (u8 i=0; i< numPathSegments_; ++i) {
          if (!CheckSegmentContinuity(i)) {
            PRINT("ERROR: Continuity check failed on segment %d\n", i);
            return false;
          }
        }
        return true;
      }
      
      // Just check specified segment
      return CheckSegmentContinuity(pathSegmentIdx);
    }
  
    
    // Add path segment
    // tODO: Change units to meters
    bool Path::AppendLine(u32 matID, f32 x_start_m, f32 y_start_m, f32 x_end_m, f32 y_end_m)
    {
      if (numPathSegments_ >= MAX_NUM_PATH_SEGMENTS) {
        PRINT("ERROR (AppendLine): Exceeded path size\n");
        return false;
      }
      
      path_[numPathSegments_].type = PST_LINE;
      path_[numPathSegments_].def.line.startPt_x = x_start_m;
      path_[numPathSegments_].def.line.startPt_y = y_start_m;
      path_[numPathSegments_].def.line.endPt_x = x_end_m;
      path_[numPathSegments_].def.line.endPt_y = y_end_m;
      
      numPathSegments_++;
      
      return true;
    }
  
  
    void Path::AddArc(f32 x_center_m, f32 y_center_m, f32 radius_m, f32 startRad, f32 sweepRad) {
      path_[numPathSegments_].type = PST_ARC;
      path_[numPathSegments_].def.arc.centerPt_x = x_center_m;
      path_[numPathSegments_].def.arc.centerPt_y = y_center_m;
      path_[numPathSegments_].def.arc.radius = radius_m;
      path_[numPathSegments_].def.arc.startRad = startRad;
      path_[numPathSegments_].def.arc.sweepRad = sweepRad;
      numPathSegments_++;
    }
  
  
    bool Path::AppendArc(u32 matID, f32 x_center_m, f32 y_center_m, f32 radius_m, f32 startRad, f32 sweepRad)
    {
      if (numPathSegments_ >= MAX_NUM_PATH_SEGMENTS) {
        PRINT("ERROR (AppendArc): Exceeded path size\n");
        return false;
      }
      
      if (FLT_NEAR(sweepRad,0)) {
        PRINT("ERROR: sweepRad is zero\n");
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
      Radians piAngle(PI);
      
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
        
        AddArc(x_center_m, y_center_m, radius_m, currAngle.ToFloat(), sweep);
          
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
    
    
    bool Path::AppendPointTurn(u32 matID, f32 x, f32 y, f32 targetAngle, f32 maxAngularVel, f32 angularAccel, f32 angularDecel)
    {
      if (numPathSegments_ >= MAX_NUM_PATH_SEGMENTS) {
        PRINT("ERROR (AppendArc): Exceeded path size\n");
        return false;
      }
      
      path_[numPathSegments_].type = PST_POINT_TURN;
      path_[numPathSegments_].def.turn.x = x;
      path_[numPathSegments_].def.turn.y = y;
      path_[numPathSegments_].def.turn.targetAngle = targetAngle;
      path_[numPathSegments_].def.turn.maxAngularVel = maxAngularVel;
      path_[numPathSegments_].def.turn.angularAccel = angularAccel;
      path_[numPathSegments_].def.turn.angularDecel = angularDecel;
      
      numPathSegments_++;
      
      return true;
    }
  
    

  } // namespace Cozmo
} // namespace Anki
