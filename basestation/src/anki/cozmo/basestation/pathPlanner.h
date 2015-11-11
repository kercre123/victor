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
#include "anki/planning/shared/path.h"
#include "clad/types/objectFamilies.h"
#include "clad/types/objectTypes.h"
#include "clad/types/pathMotionProfile.h"
#include <set>


namespace Anki {

class Pose3d;

namespace Cozmo {

enum class EComputePathStatus {
  Error, // Could not create path as requested
  Running, // Signifies that planning has successfully begun (may also be finished already)
  NoPlanNeeded, // Signifies that planning is not nessecary, useful in the replanning case
};

enum class EPlannerStatus {
  Error, // Planner encountered an error while running
  Running, // Planner is still thinking
  CompleteWithPlan, // Planner has finished, and has a valid plan
  CompleteNoPlan, // Planner has finished, but has no plan (either encountered an error, or stopped early)
};

class IPathPlanner
{
public:

  IPathPlanner();
  virtual ~IPathPlanner() {}

  // ComputePath functions start computation of a path. The underlying planner may run in a thread, or it may
  // compute immediately in the calling thread.
  // 
  // There are two versions of the function. They both take a single start pose, one takes multiple possible
  // target poses, and one takes a signle target pose. The one with multiple poses allows the planner to chose
  // which pose to drive to on its own.
  // 
  // Return value of Error indicates that there was a problem starting the plan and it isn't running. Running
  // means it is (or may have already finished)

  virtual EComputePathStatus ComputePath(const Pose3d& startPose,
                                         const Pose3d& targetPose) = 0;

  virtual EComputePathStatus ComputePath(const Pose3d& startPose,
                                         const std::vector<Pose3d>& targetPoses);

  // While we are following a path, we can do a more efficient check to see if we need to update that path
  // based on new obstacles or other information. This function assumes that the robot is following the last
  // path that was computed by ComputePath and returned by GetCompletePath. If a new path is needed, it will
  // compute it just like ComputePath. If not, it may do something more efficient to return the existing path
  // (or a portion of it). In either case, GetCompletePath should be called to return the path that should be
  // used
  // Default implementation never plans (just gives you the same path as last time)
  virtual EComputePathStatus ComputeNewPathIfNeeded(const Pose3d& startPose,
                                                    bool forceReplanFromScratch = false);

  virtual void StopPlanning() {}

  virtual EPlannerStatus CheckPlanningStatus() const;

  // returns true if it was able to copy a complete path into the passed in argument. If specified,
  // selectedTargetIndex will be set to the index of the targetPoses that was selected in the original
  // targetPoses vector passed in to ComputePath, or 0 if only one target pose was passed in to the most
  // recent ComputePlan call. If the robot has moved significantly between computing the path and getting the
  // complete path, the path may trim the first few actions removed to account for the robots motion, based on
  // the passed in position
  bool GetCompletePath(const Pose3d& currentRobotPose,
                       Planning::Path &path,
                       const PathMotionProfile* motionProfile = nullptr);
  bool GetCompletePath(const Pose3d& currentRobotPose,
                       Planning::Path &path,
                       size_t& selectedTargetIndex,
                       const PathMotionProfile* motionProfile = nullptr);

  // return a test path
  virtual void GetTestPath(const Pose3d& startPose, Planning::Path &path) {}
      
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

  bool _hasValidPath;
  bool _planningError;
  size_t _selectedTargetIdx;
  Planning::Path _path;
  
  
  virtual bool GetCompletePath_Internal(const Pose3d& currentRobotPose,
                                        Planning::Path &path);
  virtual bool GetCompletePath_Internal(const Pose3d& currentRobotPose,
                                        Planning::Path &path,
                                        size_t& selectedTargetIndex);
  
  // Modifies in path according to _pathMotionProfile to produce out path.
  // TODO: This is where Cozmo mood/skill-based path wonkification would occur,
  //       but currently it just changes speeds and accel on each segment.
  bool ApplyMotionProfile(const Planning::Path &in,
                          const PathMotionProfile& motionProfile,
                          Planning::Path &out) const;
      
}; // Interface IPathPlanner

// A stub class that never plans
class PathPlannerStub : public IPathPlanner
{
public:
  PathPlannerStub() { }

  virtual EComputePathStatus ComputePath(const Pose3d& startPose,
                                         const Pose3d& targetPose) override {
    return EComputePathStatus::Error;
  }
};
    
    
} // namespace Cozmo
} // namespace Anki


#endif // PATH_PLANNER_H
