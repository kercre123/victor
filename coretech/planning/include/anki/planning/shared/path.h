#ifndef PATH_H_
#define PATH_H_

#include "anki/common/types.h"

#define MAX_NUM_PATH_SEGMENTS 100  // TEMP: 


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
      PST_UNKNOWN = 0,
      PST_LINE,
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
      PathSegment() : type_(PST_UNKNOWN) {};
      
      // Defines the path segment as a line
      void DefineLine(f32 x_start, f32 y_start, f32 x_end, f32 y_end,
                      f32 targetSpeed, f32 accel, f32 decel);
      
      // Defines the path segment as an arc
      void DefineArc(f32 x_center, f32 y_center, f32 radius, f32 startRad, f32 sweepRad,
                     f32 targetSpeed, f32 accel, f32 decel);
      
      // Defines the path segment as a point turn
      void DefinePointTurn(f32 x, f32 y, f32 targetAngle,
                           f32 targetRotSpeed, f32 rotAccel, f32 rotDecel);
      
      // Sets the speed parameters of the current segment
      void SetSpeedProfile(f32 targetSpeed, f32 accel, f32 decel);

      // Offsets the motion by the given amount
      void OffsetStart(f32 xOffset, f32 yOffset);
      
      // Returns length of the segment
      f32 GetLength() const;
      
      void GetStartPoint(f32 &x, f32 &y) const;
      void GetEndPoint(f32 &x, f32 &y) const;
      
      void Print() const;

      SegmentRangeStatus GetDistToSegment(const f32 x, const f32 y, const f32 angle,
                                         f32 &shortestDistanceToPath, f32 &radDiff) const;
      
      PathSegmentType GetType() const {return type_;}
      PathSegmentDef& GetDef() {return def_;}
      const PathSegmentDef& GetDef() const {return def_;}
      
      f32 GetTargetSpeed() const {return targetSpeed_;}
      f32 GetAccel() const {return accel_;}
      f32 GetDecel() const {return decel_;}
      
      f32 SetTargetSpeed(const f32 speed) {return targetSpeed_ = speed;}
      f32 SetAccel(const f32 accel) {return accel_ = accel;}
      f32 SetDecel(const f32 decel) {return decel_ = decel;}

    private:
      SegmentRangeStatus GetDistToLineSegment(const f32 x, const f32 y, const f32 angle,
                                             f32 &shortestDistanceToPath, f32 &radDiff) const;
      
      SegmentRangeStatus GetDistToArcSegment(const f32 x, const f32 y, const f32 angle,
                                            f32 &shortestDistanceToPath, f32 &radDiff) const;

      SegmentRangeStatus GetDistToPointTurnSegment(const f32 x, const f32 y, const f32 angle,
                                                   f32 &shortestDistanceToPath, f32 &radDiff) const;
      
      PathSegmentType type_;
      PathSegmentDef def_;
      
      f32 targetSpeed_;  // Desired speed during segment. (In mm/s for lines and arcs. In rad/s for point turn.)
      f32 accel_;        // Acceleration with which targetSpeed may be approached. (In mm/s^2 for lines and arcs. In rad/s^2 for point turn.)
      f32 decel_;        // Deceleration with which targetSpeed may be approached. (In mm/s^2 for lines and arcs. In rad/s^2 for point turn.)
      
    };

    
    class Path
    {

    public:
      Path();
      
      void Clear(void);
      u8 GetNumSegments(void) const {return numPathSegments_;};
      
      void PrintPath() const;
      void PrintSegment(u8 segment) const;
      
      // Add path segment
      bool AppendLine(u32 matID, f32 x_start, f32 y_start, f32 x_end, f32 y_end,
                      f32 targetSpeed, f32 accel, f32 decel);
      
      bool AppendArc(u32 matID, f32 x_center, f32 y_center, f32 radius, f32 startRad, f32 sweepRad,
                     f32 targetSpeed, f32 accel, f32 decel);
      
      bool AppendPointTurn(u32 matID, f32 x, f32 y, f32 targetAngle,
                           f32 targetRotSpeed, f32 rotAccel, f32 rotDecel);

      bool AppendSegment(const PathSegment& segment);
       
      // Accessor for PathSegment at specified index
      PathSegment& operator[](const u8 idx) {return path_[idx];}
      const PathSegment& GetSegmentConstRef(const u8 idx) const {return path_[idx];}
      
      // Remove the specified number of segments from the front or back of the path.
      // Returns true if popped successfully.
      bool PopFront(const u8 numSegments);
      bool PopBack(const u8 numSegments);

      // Verifies that the path segment at the specified index
      // starts at where the previous segments ends with some tolerance
      // specified by tolerance_distance_squared.
      // If pathSegmentIdx == -1, the check is performed over the entire path
      // Returns true if continuous.
      bool CheckContinuity(f32 tolerance_distance_squared, s8 pathSegmentIdx = -1) const;
      
    private:
      
      PathSegment path_[MAX_NUM_PATH_SEGMENTS];
      u8 numPathSegments_;

      bool CheckSegmentContinuity(f32 tolerance_distance_squared, s8 pathSegmentIdx) const;
      
      void AddArc(f32 x_center, f32 y_center, f32 radius, f32 startRad, f32 sweepRad,
                  f32 targetSpeed, f32 accel, f32 decel);

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
                          f32 targetSpeed, f32 accel, f32 decel,
                          f32 final_straight_approach_length = 0,
                          f32 *path_length = 0);
   
  } // namespace Cozmo
} // namespace Anki

#endif
