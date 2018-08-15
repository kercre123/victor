#ifndef PATH_FOLLOWER_H_
#define PATH_FOLLOWER_H_

#include "coretech/planning/shared/path.h"

namespace Anki
{
  namespace Vector
  {
    namespace PathFollower
    {
      
      Result Init(void);
      
      Result Update();
      
      // Deletes current path
      void ClearPath(bool didCompletePath = false);
      
      
      // Add path segment
      // TODO: Replace these with a setPath() function
      bool AppendPathSegment_Line(f32 x_start_mm, f32 y_start_mm,
                                  f32 x_end_mm, f32 y_end_mm,
                                  f32 targetSpeed, f32 accel, f32 decel);
      
      bool AppendPathSegment_Arc(f32 x_center_mm, f32 y_center_mm,
                                 f32 radius_mm, f32 startRad, f32 sweepRad,
                                 f32 targetSpeed, f32 accel, f32 decel);
      
      bool AppendPathSegment_PointTurn(f32 x_mm, f32 y_mm,
                                       f32 startAngle, 
                                       f32 targetAngle,
                                       f32 targetRotSpeed, f32 rotAccel, f32 rotDecel,
                                       f32 angleTolerance,
                                       bool useShortestDir);
      
      u8 GenerateDubinsPath(f32 start_x, f32 start_y, f32 start_theta,
                            f32 end_x, f32 end_y, f32 end_theta,
                            f32 start_radius, f32 end_radius,
                            f32 targetSpeed, f32 accel, f32 decel,
                            f32 final_straight_approach_length = 0,
                            f32 *path_length = 0);
      
      // shortestDistanceToPath_mm: Shortest distance from robot to path
      // radDiff: The amount that the robot must turn in order to be aligned with the tangent of the closest point on the path.
      bool GetPathError(f32 &shortestDistanceToPath_mm, f32 &radDiff);
      
      void TrimPath(const u8 numPopFrontSegments, const u8 numPopBackSegments);
      
      // path_id is the ID to associate with the path that is being followed.
      // path_id of 0 is reserved for paths initiated internally by the robot
      // and does not actually update lastPathID.
      bool StartPathTraversal(u16 path_id = 0);
      bool IsTraversingPath(void);
      u16 GetLastPathID();

      // Returns const ref to the current path
      const Planning::Path& GetPath();
      
      // Returns the index of the path segment that is currently being traversed.
      // Returns -1 if not traversing a path.
      s8 GetCurrPathSegment(void);
      
      void PrintPath();
      void PrintPathSegment(s16 segment);

      
      // Convenience functions for driving a fixed distance
      bool DriveStraight(f32 dist_mm, f32 acc_start_frac, f32 acc_end_frac, f32 duration_sec);
      bool DriveArc(f32 sweep_rad, f32 radius_mm, f32 acc_start_frac, f32 acc_end_frac, f32 duration_sec);
      
      // TODO: This doesn't work so well right now because it basically uses a sequence of
      //       SteeringController::ExecutePointTurn() calls, each of which assume a terminal angular
      //       velocity of 0, so you can see the robot stopping at the end of each phase of the turn.
      //       Need to allow a terminal rotational velocity to be specified in ExecutePointTurn().
      bool DrivePointTurn(f32 angle_to_turn_rad, f32 acc_start_frac, f32 acc_end_frac, f32 angleTolerance, f32 duration_sec);
      
      
    } // namespace PathFollower
    
   
  } // namespace Vector
} // namespace Anki

#endif
