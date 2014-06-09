#ifndef PATH_FOLLOWER_H_
#define PATH_FOLLOWER_H_

#include "anki/common/types.h"

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
                                       f32 targetRotSpeed, f32 rotAccel, f32 rotDecel);
      
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
      
      bool StartPathTraversal(void);
      bool IsTraversingPath(void);

      // Returns the index of the path segment that is currently being traversed.
      // Returns -1 if not traversing a path.
      s8 GetCurrPathSegment(void);
      
      void PrintPath();
      void PrintPathSegment(s16 segment);
      
    } // namespace PathFollower
    
   
  } // namespace Cozmo
} // namespace Anki

#endif
