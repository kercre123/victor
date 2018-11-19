#ifndef PATH_H_
#define PATH_H_

#include "coretech/common/shared/types.h"
#include <string>
#include <stddef.h>

// The robot has limited memory for paths, so hold fewer segments at a
// time on the robot. The basestation will dole out the path bit by
// bit
//NOTE: these need to be even!!
#define MAX_NUM_PATH_SEGMENTS_ROBOT 128
#define MAX_NUM_PATH_SEGMENTS_BASESTATION 128


#ifdef CORETECH_ROBOT
  #ifdef CORETECH_ENGINE
  #error "only one of CORETECH_ENGINE or CORETECH_ROBOT can be defined"
  #endif
#define MAX_NUM_PATH_SEGMENTS MAX_NUM_PATH_SEGMENTS_ROBOT
#elif defined CORETECH_ENGINE
#define MAX_NUM_PATH_SEGMENTS MAX_NUM_PATH_SEGMENTS_BASESTATION
#else
#error "one of CORETECH_ENGINE or CORETECH_ROBOT must be defined"
#endif

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
        f32 endAngle;
      } line;

      // Arc
      struct s_arc {
        f32 centerPt_x;
        f32 centerPt_y;
        f32 radius;
        f32 startRad;
        f32 sweepRad;  // +ve means CCW
        f32 endAngle;
      } arc;

      // Point turn
      struct s_turn {
        f32 x;
        f32 y;
        f32 startAngle;
        f32 targetAngle;
        f32 angleTolerance;
        u8  useShortestDir;
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
      // NOTE: this is not identical to AppendArc because AppendArc may append multiple arcs
      void DefineArc(f32 x_center, f32 y_center, f32 radius, f32 startRad, f32 sweepRad,
                     f32 targetSpeed, f32 accel, f32 decel);

      // Defines the path segment as a point turn
      void DefinePointTurn(f32 x, f32 y, f32 startAngle, f32 targetAngle,
                           f32 targetRotSpeed, f32 rotAccel, f32 rotDecel,
                           f32 angleTolerance,
                           bool useShortestDir);

      // Sets the speed parameters of the current segment
      void SetSpeedProfile(f32 targetSpeed, f32 accel, f32 decel);

      // Offsets the motion by the given amount
      void OffsetStart(f32 xOffset, f32 yOffset);

      // Returns length of the segment
      f32 GetLength() const;

      f32  GetStartAngle() const;
      void GetStartPoint(f32 &x, f32 &y) const;
      void GetEndPose(f32 &x, f32 &y, f32 &angle) const;

      // Return human-friendly string representation
      std::string ToString() const;

      // Print human-friendly string to log file
      void Print() const;

      // Computes the shortest distance to the path segment from a given test point.
      //
      // @param            x, y, angle: Pose of the test point
      // @param shortestDistanceToPath: The distance to the closest point on the path segment.
      //                                +ve means the closest point is to the right of the robot assuming that
      //                                the robot is aligned with the direction of the path. (i.e. Ignores `angle` parameter)
      // @param radDiff                 The angular difference between the angle of the test point and that of the
      //                                tangent of the closest point on the path segment
      // @param distAlongSegmentFromClosestPointToEnd: Distance to end of segment from closest point on segment.
      //                                -ve means you've gone past the end of the segment
      SegmentRangeStatus GetDistToSegment(const f32 x, const f32 y, const f32 angle,
                                         f32 &shortestDistanceToPath, f32 &radDiff,
                                         f32 *distAlongSegmentFromClosestPointToEnd = NULL) const;

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
                                             f32 &shortestDistanceToPath, f32 &radDiff,
                                             f32 *distAlongSegmentFromClosestPointToEnd = NULL) const;

      SegmentRangeStatus GetDistToArcSegment(const f32 x, const f32 y, const f32 angle,
                                            f32 &shortestDistanceToPath, f32 &radDiff,
                                            f32 *distAlongSegmentFromClosestPointToEnd = NULL) const;

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
      Path(const Path& other);
      ~Path();

      Path& operator=(const Path& rhs);

      void Clear(void);
      u8 GetNumSegments(void) const {return numPathSegments_;};

      void PrintPath() const;
      void PrintSegment(u8 segment) const;

      // Add path segment
      bool AppendLine(f32 x_start, f32 y_start, f32 x_end, f32 y_end,
                      f32 targetSpeed, f32 accel, f32 decel);

      // Add one or more arcs. The arc may get split up into multiple
      // arcs so the robot can handle them more easily
      bool AppendArc(f32 x_center, f32 y_center, f32 radius, f32 startRad, f32 sweepRad,
                     f32 targetSpeed, f32 accel, f32 decel);

      bool AppendPointTurn(f32 x, f32 y, f32 startAngle, f32 targetAngle,
                           f32 targetRotSpeed, f32 rotAccel, f32 rotDecel,
                           f32 angleTolerance,
                           bool useShortestDir);

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

      PathSegment *path_;

      u8 numPathSegments_;

      // TODO:(bn) this should be enabled for debug only
      u8 capacity_;

      // This is a bit of a gigantic hack. I want to re-use this code
      // on the robot and basestation, but the path sizes are
      // different. Due to some linker weirdness, I need the object to
      // be the same size on both platforms, i.e. sizeof(Path) needs
      // to be constant. I also can't malloc on the robot, so the
      // solution is to always have a stack-allocated array large
      // enough for the robot. If I'm running on the robot, path_ will
      // point to __pathSegmentStackForRobot, otherwise it will be
      // allocated using "new" to be the correct size for the
      // basestation

      PathSegment __pathSegmentStackForRobot[MAX_NUM_PATH_SEGMENTS_ROBOT];

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

  } // namespace Planning
} // namespace Anki

#endif
