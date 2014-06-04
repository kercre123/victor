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
#include "json/json-forwards.h"

namespace Anki {
  namespace Cozmo {

    class BlockWorld;
    
    class IPathPlanner
    {
    public:
      virtual Result GetPlan(Planning::Path &path, const Pose3d &startPose, const Pose3d &targetPose) = 0;
      
    }; // Interface IPathPlanner

    // This is the Dubbins planner
    class PathPlanner : public IPathPlanner
    {
    public:
      PathPlanner();
      
      virtual Result GetPlan(Planning::Path &path, const Pose3d &startPose, const Pose3d &targetPose);
      
    protected:
      
      
    }; // class PathPlanner

  class LatticePlannerImpl;
    class LatticePlanner : public IPathPlanner
    {
    public:
      LatticePlanner(const BlockWorld* blockWorld, const Json::Value& mprims);
      virtual ~LatticePlanner();
      
      virtual Result GetPlan(Planning::Path &path, const Pose3d &startPose, const Pose3d &targetPose);

    protected:
      LatticePlannerImpl* impl_;
    };
    
    class PathPlannerStub : public IPathPlanner
    {
    public:
      PathPlannerStub() { }
      
      virtual Result GetPlan(Planning::Path &path, const Pose3d &startPose, const Pose3d &targetPose) {
        return RESULT_OK;
      }
      
    }; // class PathPlannerStub
    
    
  } // namespace Cozmo
} // namespace Anki


#endif // PATH_PLANNER_H
