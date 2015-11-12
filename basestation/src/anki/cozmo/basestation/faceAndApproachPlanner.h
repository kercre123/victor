/**
 * File: faceAndApproachPlanner.h
 *
 * Author: Brad Neuman
 * Created: 2015-09-16
 *
 * Description: A simple "planner" which will do a turn in place, followed by a straight action, followed by a
 * final turn in point.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __FACEANDAPPROACHPLANNER_H__
#define __FACEANDAPPROACHPLANNER_H__

#include "anki/common/basestation/math/point.h"
#include "pathPlanner.h"

namespace Anki {
namespace Cozmo {

class FaceAndApproachPlanner : public IPathPlanner
{
public:

  virtual EComputePathStatus ComputePath(const Pose3d& startPose,
                                         const Pose3d& targetPose) override;

  virtual EComputePathStatus ComputeNewPathIfNeeded(const Pose3d& startPose,
                                                    bool forceReplanFromScratch = false) override;
protected:
  Vec3f _targetVec;
  float _finalTargetAngle;
};

}
}


#endif
