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
      
  virtual EPlanStatus GetPlan(Planning::Path &path,
                              const Pose3d& startPose,
                              bool forceReplanFromScratch = false) override;

  virtual EPlanStatus GetPlan(Planning::Path &path,
                              const Pose3d& startPose,
                              const Pose3d& targetPose) override;

  // This version gets a plan to any of the goals you supply. It
  // is up to the planner implementation to decide 
  virtual EPlanStatus GetPlan(Planning::Path &path,
                              const Pose3d& startPose,
                              const std::vector<Pose3d>& targetPoses,
                              size_t& selectedIndex) override;

  virtual void GetTestPath(const Pose3d& startPose, Planning::Path &path) override;

protected:
  LatticePlannerImpl* _impl;
};

}
}

#endif
