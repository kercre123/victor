#ifndef PATH_H_
#define PATH_H_

#include "anki/common/types.h"
#include "anki/cozmo/robot/cozmoTypes.h"

#define MAX_NUM_PATH_SEGMENTS 10


namespace Anki
{
  namespace Cozmo
  {
    
    typedef enum {
      LSL = 0,
      LSR,
      RSL,
      RSR,
      LRL,
      RLR,
      NUM_DUBINS_PATHS
    } DubinsPathType;
    
    
    typedef enum {
      PST_LINE,
      PST_ARC,
      PST_POINT_TURN
    } PathSegmentType;
    
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
        //f32 endRad;
        //bool movingCCW;
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
    
    
    class PathSegment
    {
    public:
      PathSegment(){};
      
      Anki::Embedded::Point2f GetStartPoint() const;
      Anki::Embedded::Point2f GetEndPoint() const;
      
      void Print() const;
      
      PathSegmentType type;
      PathSegmentDef def;
      
      s16 desiredSpeed_mmPerSec;  // Desired speed during segment
      s16 terminalSpeed_mmPerSec; // Desired speed by the time we reach the end of the segment
      
    };

    
    class Path
    {

    public:
      Path();
      
      void Clear(void);
      int GetNumSegments(void) const;
      
      void PrintPath() const;
      void PrintSegment(u8 segment) const;
      
      
      // Add path segment
      bool AppendLine(u32 matID, f32 x_start_m, f32 y_start_m, f32 x_end_m, f32 y_end_m);
      bool AppendArc(u32 matID, f32 x_center_m, f32 y_center_m, f32 radius_m, f32 startRad, f32 sweepRad);
      bool AppendPointTurn(u32 matID, f32 x, f32 y, f32 targetAngle, f32 maxAngularVel, f32 angularAccel, f32 angularDecel);
      
      // Generate Dubins path
      u8 GenerateDubinsPath(f32 start_x, f32 start_y, f32 start_theta,
                            f32 end_x, f32 end_y, f32 end_theta,
                            f32 start_radius, f32 end_radius);

      // Accessor for PathSegment at specified index
      const PathSegment& operator[](const u8 idx) {return path_[idx];}

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
      f32 GetArcAngle(f32 start_x, f32 start_y, f32 end_x, f32 end_y, f32 center_x, f32 center_y, bool CCW);
    };
    
   
  } // namespace Cozmo
} // namespace Anki

#endif
