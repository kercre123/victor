/**
 * File: pathPlanner.h
 *
 * Author: Kevin Yoon
 * Date:   2/24/2014
 *
 * Description: Interface for path planners
 *
 * Copyright: Anki, Inc. 2014-2015
 **/

#ifndef __PATH_PLANNER_H__
#define __PATH_PLANNER_H__

#include "anki/common/basestation/objectIDs.h"
#include "clad/types/objectFamilies.h"
#include "clad/types/objectTypes.h"
#include <set>


namespace Anki {

class Pose3d;

namespace Planning {
class Path;
}

namespace Cozmo {

class IPathPlanner
{
public:

  IPathPlanner() {}      
  virtual ~IPathPlanner() {}

  // Replan if needed because the environment changed. Returns DID_REPLAN if there is a new path and
  // REPLAN_NOT_NEEDED if no replan was necessary and the path has not changed.  If a new path is needed but
  // could not be computed a corresponding enum value is returned.  Assumes the goal pose didn't change. If
  // forceReplanFromScratch = true, then definitely do a new plan, from scratch.
  // 
  // If the goal hasn't changed, it is better to call the version
  // that doesn't specify a goal
  // 
  // NOTE: Some planners may never attempt to replan unless you set forceReplanFromScratch
  enum EPlanStatus {
    PLAN_NOT_NEEDED,
    DID_PLAN,
    PLAN_NEEDED_BUT_START_FAILURE,
    PLAN_NEEDED_BUT_GOAL_FAILURE,
    PLAN_NEEDED_BUT_PLAN_FAILURE
  };

  virtual EPlanStatus GetPlan(Planning::Path &path,
                              const Pose3d& startPose,
                              bool forceReplanFromScratch = false) {return PLAN_NOT_NEEDED;};

  // A simple planner that doesn't really support replanning can just implement this
  // function. forceReplanFromScratch is implied to be true because we are changing both the start and the
  // goal
  virtual EPlanStatus GetPlan(Planning::Path &path,
                              const Pose3d& startPose,
                              const Pose3d& targetPose) = 0;
     
  // This version gets a plan to the closest of the goals you supply. It is up to the planner implementation
  // to override and choose based on other criteria. The index of the selected target pose is returned in the
  // last argument.
  virtual EPlanStatus GetPlan(Planning::Path &path,
                              const Pose3d& startPose,
                              const std::vector<Pose3d>& targetPoses,
                              size_t& selectedIndex);

  // return a test path
  virtual void GetTestPath(const Pose3d& startPose, Planning::Path &path) override {}
      
  void AddIgnoreFamily(const ObjectFamily objFamily)    { _ignoreFamilies.insert(objFamily); }
  void RemoveIgnoreFamily(const ObjectFamily objFamily) { _ignoreFamilies.erase(objFamily); }
  void ClearIgnoreFamilies()                                          { _ignoreFamilies.clear(); }
      
  void AddIgnoreType(const ObjectType objType)    { _ignoreTypes.insert(objType); }
  void RemoveIgnoreType(const ObjectType objType) { _ignoreTypes.erase(objType); }
  void ClearIgnoreTypes()                           { _ignoreTypes.clear(); }
      
  void AddIgnoreID(const ObjectID objID)          { _ignoreIDs.insert(objID); }
  void RemoveIgnoreID(const ObjectID objID)       { _ignoreIDs.erase(objID); }
  void ClearIgnoreIDs()                             { _ignoreIDs.clear(); }
      
protected:
      
  std::set<ObjectFamily>             _ignoreFamilies;
  std::set<ObjectType>               _ignoreTypes;
  std::set<ObjectID>                 _ignoreIDs;
      
}; // Interface IPathPlanner

// A stub class that never plans
class PathPlannerStub : public IPathPlanner
{
public:
  PathPlannerStub() { }

  virtual EPlanStatus GetPlan(Planning::Path &path,
                              const Pose3d& startPose,
                              const Pose3d& targetPose) override {
    return PLAN_NOT_NEEDED;
  }
};
    
    
} // namespace Cozmo
} // namespace Anki


#endif // PATH_PLANNER_H
