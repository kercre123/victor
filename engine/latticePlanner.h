/**
 * File: latticePlanner.h
 *
 * Author: Brad Neuman
 * Created: 2015-09-16
 *
 * Description: This is the cozmo interface to the xytheta lattice planner in coretech. It does full collision
 * avoidance and planning in (x,y,theta) space
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __LATTICEPLANNER_H__
#define __LATTICEPLANNER_H__

#include "json/json-forwards.h"
#include "pathPlanner.h"
#include "coretech/planning/shared/goalDefs.h"
#include "util/helpers/noncopyable.h"
#include <vector>

namespace Anki {

namespace Util {
namespace Data {
class DataPlatform;
}
}

namespace Cozmo {

class LatticePlannerInternal;
class Path;
class Robot;

class LatticePlanner : public IPathPlanner, private Util::noncopyable
{
  friend LatticePlannerInternal;
public:
  LatticePlanner(Robot* robot, Util::Data::DataPlatform* dataPlatform);
  virtual ~LatticePlanner();

  virtual EComputePathStatus ComputePath(const Pose3d& startPose,
                                         const std::vector<Pose3d>& targetPoses) override;

  virtual EComputePathStatus ComputeNewPathIfNeeded(const Pose3d& startPose,
                                                    bool forceReplanFromScratch = false,
                                                    bool allowGoalChange = true) override;
  
  virtual bool PreloadObstacles() override;
  
  virtual bool CheckIsPathSafe(const Planning::Path& path, float startAngle, Planning::Path& validPath) const override;
  
  virtual bool ChecksForCollisions() const override { return true; }

  virtual void StopPlanning() override;

  // TODO:(bn) remove this from IPathPlanner! Move to PathComponent
  virtual void GetTestPath(const Pose3d& startPose,
                           Planning::Path &path,
                           const PathMotionProfile* motionProfile = nullptr) override;

  virtual EPlannerStatus CheckPlanningStatus() const override;

  // by default, this planner will run in a thread. If it is set to be synchronous, it will not
  void SetIsSynchronous(bool val);

  // Useful for testing, tell the planner to sleep for the given number of milliseconds after planning before
  // any results are processed (this is skipped if the planner fails for any reason)
  void SetArtificialPlannerDelay_ms(int ms);

protected:
  virtual EComputePathStatus ComputePath(const Pose3d& startPose,
                                         const Pose3d& targetPose) override;

  EComputePathStatus ComputePathHelper(const Pose3d& startPose,
                                       const std::vector<Pose3d>& targetPoses);

  virtual bool GetCompletePath_Internal(const Pose3d& currentRobotPose, Planning::Path &path) override;
  virtual bool GetCompletePath_Internal(const Pose3d& currentRobotPose,
                                        Planning::Path &path,
                                        Planning::GoalID& selectedTargetIndex) override;
  LatticePlannerInternal* _impl;
};

}
}

#endif
