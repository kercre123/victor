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
      bool AppendPathSegment_Line(u32 matID, float x_start_m, float y_start_m, float x_end_m, float y_end_m);
      bool AppendPathSegment_Arc(u32 matID, float x_center_m, float y_center_m, float radius_m, float startRad, float endRad);
      
      int GetNumPathSegments(void);
      
      bool GetPathError(float &shortestDistanceToPath_m, float &radDiff);
      
      bool StartPathTraversal(void);
      bool IsTraversingPath(void);
      
      
      // Simulation debug
      void EnablePathVisualization(bool on);
      
    } // namespace PathFollower
  } // namespace Cozmo
} // namespace Anki

#endif
