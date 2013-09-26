#ifndef PATH_FOLLOWER_H_
#define PATH_FOLLOWER_H_

#include "cozmoTypes.h"


namespace PathFollower
{

  void InitPathFollower(void);


  // Deletes current path
  void ClearPath(void);


  // Add path segment
  BOOL AppendPathSegment_Line(u32 matID, float x_start_m, float y_start_m, float x_end_m, float y_end_m);
  BOOL AppendPathSegment_Arc(u32 matID, float x_center_m, float y_center_m, float radius_m, float startRad, float endRad);

  int GetNumPathSegments(void);

  BOOL GetPathError(float &shortestDistanceToPath_m, float &radDiff);
  
  BOOL StartPathTraversal(void);
  BOOL IsTraversingPath(void);


  // Simulation debug
  void EnablePathVisualization(BOOL on);
}
#endif
