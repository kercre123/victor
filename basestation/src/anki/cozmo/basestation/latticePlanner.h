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
#include "util/helpers/noncopyable.h"
#include <vector>

namespace Anki {

namespace Util {
namespace Data {
class DataPlatform;
}
}

namespace Cozmo {

class LatticePlannerImpl;
class Path;
class Robot;

class LatticePlanner : public IPathPlanner, private Util::noncopyable
{
  friend LatticePlannerImpl;
public:
  LatticePlanner(const Robot* robot, Util::Data::DataPlatform* dataPlatform);
  virtual ~LatticePlanner();


  virtual EComputePathStatus ComputePath(const Pose3d& startPose,
                                         const Pose3d& targetPose,
                                         const PathMotionProfile motionProfile) override;

  virtual EComputePathStatus ComputePath(const Pose3d& startPose,
                                         const std::vector<Pose3d>& targetPoses,
                                         const PathMotionProfile motionProfile) override;

  virtual EComputePathStatus ComputeNewPathIfNeeded(const Pose3d& startPose,
                                                    bool forceReplanFromScratch = false,
                                                    const PathMotionProfile* motionProfile = nullptr) override;

  virtual void StopPlanning() override;

  virtual bool GetCompletePath(const Pose3d& currentRobotPose, Planning::Path &path) override;
  virtual bool GetCompletePath(const Pose3d& currentRobotPose,
                               Planning::Path &path,
                               size_t& selectedTargetIndex) override;

  virtual void GetTestPath(const Pose3d& startPose, Planning::Path &path) override;

  virtual EPlannerStatus CheckPlanningStatus() const override;

protected:
  EComputePathStatus ComputePathHelper(const Pose3d& startPose,
                                       const Pose3d& targetPose,
                                       const PathMotionProfile* motionProfile = nullptr);
  
  LatticePlannerImpl* _impl;
};

}
}

#endif
