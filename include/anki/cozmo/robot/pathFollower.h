#ifndef PATH_FOLLOWER_H_
#define PATH_FOLLOWER_H_

#include "anki/common/types.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace PathFollower
    {
      
      ReturnCode Init(void);
      
      ReturnCode Update();
      
      // Deletes current path
      void ClearPath(void);
      
      
      // Add path segment
      // TODO: Replace these with a setPath() function
      bool AppendPathSegment_Line(u32 matID,
                                  f32 x_start_mm, f32 y_start_mm,
                                  f32 x_end_mm, f32 y_end_mm);
      
      bool AppendPathSegment_Arc(u32 matID,
                                 f32 x_center_mm, f32 y_center_mm,
                                 f32 radius_mm, f32 startRad, f32 sweepRad);
      
      bool AppendPathSegment_PointTurn(u32 matID,
                                       f32 x_mm, f32 y_mm,
                                       f32 targetAngle,
                                       f32 maxAngularVel,
                                       f32 angularAccel,
                                       f32 angularDecel);
      
      u8 GenerateDubinsPath(f32 start_x, f32 start_y, f32 start_theta,
                            f32 end_x, f32 end_y, f32 end_theta,
                            f32 start_radius, f32 end_radius,
                            f32 final_straight_approach_length = 0,
                            f32 *path_length = 0);
      
      bool GetPathError(f32 &shortestDistanceToPath_mm, f32 &radDiff);
      
      bool StartPathTraversal(void);
      bool IsTraversingPath(void);
      
      void PrintPath();
      void PrintPathSegment(s16 segment);
      
      // Simulation debug
      void EnablePathVisualization(bool on);
      
    } // namespace PathFollower
    
   
  } // namespace Cozmo
} // namespace Anki

#endif
