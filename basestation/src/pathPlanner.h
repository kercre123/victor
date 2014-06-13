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

#include <set>

namespace Anki {
  namespace Cozmo {

    class BlockWorld;
    
    class IPathPlanner
    {
    public:
      virtual Result GetPlan(Planning::Path &path, const Pose3d &startPose, const Pose3d &targetPose) = 0;

      // Replan if needed because the environment changed. Returns
      // true if there is a new path, otherwise it doesn't update the
      // path and returns false. Assumes the goal pose didn't change
      virtual bool ReplanIfNeeded(Planning::Path &path, const Pose3d& startPose) {return false;};
      
      void AddIgnoreType(const ObjectType_t objType)    { _ignoreTypes.insert(objType); }
      void RemoveIgnoreType(const ObjectType_t objType) { _ignoreTypes.erase(objType); }
      void ClearIgnoreTypes()                           { _ignoreTypes.clear(); }
      
      void AddIgnoreID(const ObjectID_t objID)          { _ignoreIDs.insert(objID); }
      void RemoveIgnoreID(const ObjectID_t objID)       { _ignoreIDs.erase(objID); }
      void ClearIgnoreIDs()                             { _ignoreIDs.clear(); }
      
    protected:
      
      std::set<ObjectType_t> _ignoreTypes;
      std::set<ObjectID_t>   _ignoreIDs;
      
    }; // Interface IPathPlanner

    // This is the Dubbins planner
    class PathPlanner : public IPathPlanner
    {
    public:
      PathPlanner();
      
      virtual Result GetPlan(Planning::Path &path, const Pose3d &startPose, const Pose3d &targetPose) override;
      
    protected:
      
      
    }; // class PathPlanner

    class LatticePlannerImpl;

    class LatticePlanner : public IPathPlanner
    {
    public:
      LatticePlanner(const BlockWorld* blockWorld, const Json::Value& mprims);
      virtual ~LatticePlanner();
      
      virtual Result GetPlan(Planning::Path &path, const Pose3d &startPose, const Pose3d &targetPose) override;

      virtual bool ReplanIfNeeded(Planning::Path &path, const Pose3d& startPose) override;

    protected:
      LatticePlannerImpl* impl_;
    };
    
    class PathPlannerStub : public IPathPlanner
    {
    public:
      PathPlannerStub() { }
      
      virtual Result GetPlan(Planning::Path &path, const Pose3d &startPose, const Pose3d &targetPose) override {
        return RESULT_OK;
      }
      
    }; // class PathPlannerStub
    
    
  } // namespace Cozmo
} // namespace Anki


#endif // PATH_PLANNER_H
