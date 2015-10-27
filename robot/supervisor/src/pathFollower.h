#ifndef PATH_FOLLOWER_H_
#define PATH_FOLLOWER_H_

#include "anki/types.h"
#include "anki/planning/shared/path.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace PathFollower
    {
      
      Result Init(void);
      
      Result Update();
      
      // Deletes current path
      void ClearPath(void);
      
      
      // Add path segment
      // TODO: Replace these with a setPath() function
      bool AppendPathSegment_Line(u32 matID,
                                  f32 x_start_mm, f32 y_start_mm,
                                  f32 x_end_mm, f32 y_end_mm,
                                  f32 targetSpeed, f32 accel, f32 decel);
      
      bool AppendPathSegment_Arc(u32 matID,
                                 f32 x_center_mm, f32 y_center_mm,
                                 f32 radius_mm, f32 startRad, f32 sweepRad,
                                 f32 targetSpeed, f32 accel, f32 decel);
      
      bool AppendPathSegment_PointTurn(u32 matID,
                                       f32 x_mm, f32 y_mm,
                                       f32 targetAngle,
                                       f32 targetRotSpeed, f32 rotAccel, f32 rotDecel,
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
      // manualSpeedControl: Set to true if you want to control the speed at which
      // the path is traversed via SetManualPathSpeed() vs using the speed values
      // embedded in the path.
      bool StartPathTraversal(u16 path_id = 0, bool manualSpeedControl = false);
      bool IsTraversingPath(void);
      u16 GetLastPathID();

      // Returns const ref to the current path
      const Planning::Path& GetPath();
      
      // Whether or not the speed at which to traverse path is determined by the speeds
      // set by SetManualPathSpeed()
      bool IsInManualSpeedMode();
      
      // If a path has been started with manualSpeedControl == true, then the path will be
      // traversed at these speed settings rather than the settings embedded in the path.
      void SetManualPathSpeed(f32 speed_mmps, f32 accel_mmps2, f32 decel_mmps2);
      
      // Returns the index of the path segment that is currently being traversed.
      // Returns -1 if not traversing a path.
      s8 GetCurrPathSegment(void);

      // Returns the number of available segment slots.
      // This is how many more path segments can be received by the basestation.
      u8 GetNumFreeSegmentSlots();
      
      void PrintPath();
      void PrintPathSegment(s16 segment);

      
      // Convenience functions for driving a fixed distance
      bool DriveStraight(f32 dist_mm, f32 acc_start_frac, f32 acc_end_frac, f32 duration_sec);
      bool DriveArc(f32 sweep_rad, f32 radius_mm, f32 acc_start_frac, f32 acc_end_frac, f32 duration_sec);
      
      // TODO: This doesn't work so well right now because it basically uses a sequence of
      //       SteeringController::ExecutePointTurn() calls, each of which assume a terminal angular
      //       velocity of 0, so you can see the robot stopping at the end of each phase of the turn.
      //       Need to allow a terminal rotational velocity to be specified in ExecutePointTurn().
      bool DrivePointTurn(f32 angle_to_turn_rad, f32 acc_start_frac, f32 acc_end_frac, f32 duration_sec);
      
      
    } // namespace PathFollower
    
   
  } // namespace Cozmo
} // namespace Anki

#endif
