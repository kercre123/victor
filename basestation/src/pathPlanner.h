/**
 * File: pathPlanner.h
 *
 * Author: Kevin Yoon
 * Date:   2/24/2014
 *
 * Description: Class for planning collision-free paths from one pose to another.
 *              (Currently, uses Dubins path planning without any collision checking - Kevin 2/24/2014)
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef PATH_PLANNER_H
#define PATH_PLANNER_H

#include "anki/common/basestation/math/pose.h"
#include "anki/common/types.h"
#include "anki/planning/shared/path.h"

namespace Anki {
  namespace Cozmo {
    
    class PathPlanner
    {
    public:
      PathPlanner();
      
      Result GetPlan(Planning::Path &path, const Pose3d &startPose, const Pose3d &targetPose);
      
    protected:
      
      
    }; // class PathPlanner
    
    
  } // namespace Cozmo
} // namespace Anki


#endif // PATH_PLANNER_H