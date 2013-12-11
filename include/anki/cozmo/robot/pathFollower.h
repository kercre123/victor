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
      bool AppendPathSegment_Line(u32 matID, f32 x_start_m, f32 y_start_m, f32 x_end_m, f32 y_end_m);
      bool AppendPathSegment_Arc(u32 matID, f32 x_center_m, f32 y_center_m, f32 radius_m, f32 startRad, f32 sweepRad);
      bool AppendPathSegment_PointTurn(u32 matID, f32 targetAngle, f32 maxAngularVel, f32 angularAccel, f32 angularDecel);
      
      
      bool GetPathError(f32 &shortestDistanceToPath_m, f32 &radDiff);
      
      bool StartPathTraversal(void);
      bool IsTraversingPath(void);
      
      void PrintPathSegment(s16 segment);
      
      // Simulation debug
      void EnablePathVisualization(bool on);
      
    } // namespace PathFollower
    
   
  } // namespace Cozmo
} // namespace Anki

#endif
