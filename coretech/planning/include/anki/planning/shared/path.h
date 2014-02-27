#ifndef PATH_H_
#define PATH_H_

#include "anki/common/types.h"

#define MAX_NUM_PATH_SEGMENTS 10


namespace Anki
{
  namespace Planning
  {
    
    typedef enum {
      LSL = 0,
      LSR,
      RSL,
      RSR,
      NUM_DUBINS_PATHS
    } DubinsPathType;
    
    
    typedef enum {
      PST_LINE = 0,
      PST_ARC,
      PST_POINT_TURN
    } PathSegmentType;
    
    
    // TODO: The following structs and class might be better separated into a robot version and basestation version.
    
    typedef union {
      // Line
      struct s_line {
        f32 startPt_x;
        f32 startPt_y;
        f32 endPt_x;
        f32 endPt_y;
      } line;
      
      // Arc
      struct s_arc {
        f32 centerPt_x;
        f32 centerPt_y;
        f32 radius;
        f32 startRad;
        f32 sweepRad;  // +ve means CCW
      } arc;
      
      // Point turn
      struct s_turn {
        f32 x;
        f32 y;
        f32 targetAngle;
        f32 maxAngularVel;
        f32 angularAccel;
        f32 angularDecel;
      } turn;
    } PathSegmentDef;
    
    typedef enum {
      IN_SEGMENT_RANGE,
      OOR_NEAR_START,
      OOR_NEAR_END
    } SegmentRangeStatus;
    
    
    class PathSegment
    {
    public:
      PathSegment(){};
      
      ReturnCode GetStartPoint(f32 &x, f32 &y) const;
      ReturnCode GetEndPoint(f32 &x, f32 &y) const;
      
      void Print() const;

      SegmentRangeStatus GetDistToSegment(const f32 x, const f32 y, const f32 angle,
                                         f32 &shortestDistanceToPath_m, f32 &radDiff) const;
      
      
      PathSegmentType type;
      PathSegmentDef def;
      
      s16 desiredSpeed_mmPerSec;  // Desired speed during segment
      s16 terminalSpeed_mmPerSec; // Desired speed by the time we reach the end of the segment

    private:
      SegmentRangeStatus GetDistToLineSegment(const f32 x, const f32 y, const f32 angle,
                                             f32 &shortestDistanceToPath_m, f32 &radDiff) const;
      
      SegmentRangeStatus GetDistToArcSegment(const f32 x, const f32 y, const f32 angle,
                                            f32 &shortestDistanceToPath_m, f32 &radDiff) const;
      

    };

    
    class Path
    {

    public:
      Path();
      
      void Clear(void);
      int GetNumSegments(void) const {return numPathSegments_;};
      
      void PrintPath() const;
      void PrintSegment(u8 segment) const;
      
      
      // Add path segment
      bool AppendLine(u32 matID, f32 x_start_m, f32 y_start_m, f32 x_end_m, f32 y_end_m);
      bool AppendArc(u32 matID, f32 x_center_m, f32 y_center_m, f32 radius_m, f32 startRad, f32 sweepRad);
      bool AppendPointTurn(u32 matID, f32 x, f32 y, f32 targetAngle, f32 maxAngularVel, f32 angularAccel, f32 angularDecel);
       
      // Accessor for PathSegment at specified index
      const PathSegment& operator[](const u8 idx) const {return path_[idx];}

      // Verifies that the path segment at the specified index
      // starts at where the previous segments ends.
      // If pathSegmentIdx == -1, the check is performed over the entire path
      // Returns true if continuous.
      bool CheckContinuity(s16 pathSegmentIdx = -1) const;
      
    private:
      
      PathSegment path_[MAX_NUM_PATH_SEGMENTS];
      s16 numPathSegments_;

      bool CheckSegmentContinuity(u8 pathSegmentIdx) const;
      
      void AddArc(f32 x_center_m, f32 y_center_m, f32 radius_m, f32 startRad, f32 sweepRad);

      // Returns angle between two points on a circle
      //f32 GetArcAngle(f32 start_x, f32 start_y, f32 end_x, f32 end_y, f32 center_x, f32 center_y, bool CCW);
      
      // Generates a CSC Dubins curve if one exists.
      // Returns the number of segments in the path.
      //u8 GenerateCSCCurve(f32 startPt_x, f32 startPt_y, f32 startPt_theta,
      //                    f32 endPt_x, f32 endPt_y, f32 endPt_theta,
      //                    f32 start_radius, f32 end_radius,
      //                    DubinsPathType pathType, PathSegment path[], f32 &path_length);
    };
    
    
    
    
    /////////// Path Planners ////////////////
    
    // Generate a curve-straight-curve Dubins path from start pose to end pose
    // with the specified radii as parameters for the curved segments.
    //
    // final_straight_approach_length: Length of straight line segment to
    // append to the path. Useful for docking where alignment at the end
    // of the path is important.
    //
    // path_length: Length of shortest Dubins path generated.
    //
    // Returns number of segments in the path.
    u8 GenerateDubinsPath(Path &path,
                          f32 start_x, f32 start_y, f32 start_theta,
                          f32 end_x, f32 end_y, f32 end_theta,
                          f32 start_radius, f32 end_radius,
                          f32 final_straight_approach_length = 0,
                          f32 *path_length = 0);
   
  } // namespace Cozmo
} // namespace Anki

#endif
