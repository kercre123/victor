#include "anki/cozmo/robot/path.h"
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
          p.x = def.arc.centerPt_x + def.arc.radius * cos(def.arc.startRad);
          p.y = def.arc.centerPt_y + def.arc.radius * sin(def.arc.startRad);
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
          p.x = def.arc.centerPt_x + def.arc.radius * cos(def.arc.startRad + def.arc.sweepRad);
          p.y = def.arc.centerPt_y + def.arc.radius * sin(def.arc.startRad + def.arc.sweepRad);
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
    
    
    /////////////// Path implementations ////////////////

    
    void Path::PrintPath() const
    {
      for(u8 i = 0; i<numPathSegments_; ++i) {
        PrintSegment(i);
      }
    }
    
    void Path::PrintSegment(u8 segment) const
    {
        
        if (segment < numPathSegments_) {
          
          switch(path_[segment].type) {
            case PST_LINE:
            PRINT("Path segment %d (line): (%f, %f) to (%f, %f)\n",
                  segment,
                  path_[segment].def.line.startPt_x,
                  path_[segment].def.line.startPt_y,
                  path_[segment].def.line.endPt_x,
                  path_[segment].def.line.endPt_y);
              break;
            case PST_ARC:
              PRINT("Path segment %d (arc): centerPt (%f, %f), radius %f, startAng %f, sweep %f\n",
                    segment,
                    path_[segment].def.arc.centerPt_x,
                    path_[segment].def.arc.centerPt_y,
                    path_[segment].def.arc.radius,
                    path_[segment].def.arc.startRad,
                    path_[segment].def.arc.sweepRad);
              break;
            case PST_POINT_TURN:
              PRINT("Path segment %d (point turn): targetAngle %f, maxAngularVel %f, angularAccel %f, angularDecel %f\n",
                    segment,
                    path_[segment].def.turn.targetAngle,
                    path_[segment].def.turn.maxAngularVel,
                    path_[segment].def.turn.angularAccel,
                    path_[segment].def.turn.angularDecel);
              break;
          }
        }
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
                              f32 start_radius, f32 end_radius)
      {
        
        // Find transform between start and end pose
        
        // Rotate end point about start point by start_theta to get endPt, which is the end point in the frame of the start pose.
        f32 dx = end_x - start_x;
        f32 dy = end_y - start_y;
        
        f32 endPt_x = cos(-start_theta) * dx - sin(-start_theta) * dy;
        f32 endPt_y = sin(-start_theta) * dx + cos(-start_theta) * dy;
        f32 endPt_theta = end_theta - start_theta;
        
        PRINT("DUBINS: endPt %f %f %f\n", endPt_x, endPt_y, endPt_theta);
 
    
        
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
        
  
        
        PathSegment csc_path[NUM_DUBINS_PATHS][3];
        f32 r1 = start_radius;
        f32 r2 = end_radius;
        f32 p_c1_x, p_c1_y;
        f32 p_c2_x, p_c2_y;
        f32 p_t1_x, p_t1_y;
        f32 p_t2_x, p_t2_y;
        f32 V1_x, V1_y, V2_x, V2_y, n_x, n_y;
        f32 v1_x, v1_y;
        f32 V1_mag;
        f32 cosTanPtAngle, sinTanPtAngle, tanPtAngle;
        
        // ==== RSR ====
        p_c1_x = 0;
        p_c1_y = -r1;
        
        // Compute p_c2
        p_c2_x = endPt_x + r2 * sin(endPt_theta);
        p_c2_y = endPt_y - r2 * cos(endPt_theta);
        
        // Compute V1
        V1_x = p_c2_x - p_c1_x;
        V1_y = p_c2_y - p_c1_y;
        V1_mag = sqrt(V1_x * V1_x + V1_y * V1_y);

        v1_x = V1_x / V1_mag;
        v1_y = V1_y / V1_mag;
        
        // Compute cosTanPtAngle (aka c) and sinTanPtAngle
        cosTanPtAngle = (r1 - r2) / V1_mag;
        sinTanPtAngle = sqrt(1-cosTanPtAngle*cosTanPtAngle);
        
        // Compute n
        n_x = v1_x * cosTanPtAngle - v1_y * sinTanPtAngle;
        n_y = v1_x * sinTanPtAngle + v1_y * cosTanPtAngle;
        
        tanPtAngle = atan2_acc(n_y, n_x);
        
        // Compute tangent points
        p_t1_x = p_c1_x + n_x * r1;
        p_t1_y = p_c1_y + n_y * r1;
        
        p_t2_x = p_c2_x + n_x * r2;
        p_t2_x = p_c2_x + n_y * r2;
        
        
        // Generate path segment definitions
        // Arc1
        PathSegment* ps = &csc_path[RSR][0];
        ps->type = PST_ARC;
        ps->def.arc.centerPt_x = p_c1_x;
        ps->def.arc.centerPt_y = p_c1_y;
        ps->def.arc.startRad = PIDIV2_F;
//        ps->def.arc.endRad = ;
        
        
        
        // TODO: Compute LRL and RLR?
        
        return 0;
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
        path_[numPathSegments_].def.turn.x = y;
        path_[numPathSegments_].def.turn.targetAngle = targetAngle;
        path_[numPathSegments_].def.turn.maxAngularVel = maxAngularVel;
        path_[numPathSegments_].def.turn.angularAccel = angularAccel;
        path_[numPathSegments_].def.turn.angularDecel = angularDecel;
        
        numPathSegments_++;
        
        return true;
      }
    
      

  } // namespace Cozmo
} // namespace Anki
