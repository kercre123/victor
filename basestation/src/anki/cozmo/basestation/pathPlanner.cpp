/**
 * File: pathPlanner.cpp
 *
 * Author: Kevin Yoon
 * Date:   2/24/2014
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/math/point_impl.h"
#include "pathPlanner.h"
#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {

    
IPathPlanner::EPlanStatus IPathPlanner::GetPlan(Planning::Path &path,
                                                const Pose3d& startPose,
                                                const std::vector<Pose3d>& targetPoses,
                                                size_t& selectedIndex)
{
  // Select the closest
  const Pose3d* closestPose = nullptr;
      
  selectedIndex = 0;
  bool foundTarget = false;
  f32 shortestDistToPose = -1.f;
  for(size_t i=0; i<targetPoses.size(); ++i)
  {
    const Pose3d& targetPose = targetPoses[i];
        
    PRINT_NAMED_INFO("IPathPlanner.GetPlan.FindClosestPose",
                     "Candidate target pose: (%.2f %.2f %.2f), %.1fdeg @ (%.2f %.2f %.2f)\n",
                     targetPose.GetTranslation().x(),
                     targetPose.GetTranslation().y(),
                     targetPose.GetTranslation().z(),
                     targetPose.GetRotationAngle<'Z'>().getDegrees(),
                     targetPose.GetRotationAxis().x(),
                     targetPose.GetRotationAxis().y(),
                     targetPose.GetRotationAxis().z());
        
    const f32 distToPose = (targetPose.GetTranslation() - startPose.GetTranslation()).LengthSq();
    if (!foundTarget || distToPose < shortestDistToPose)
    {
      foundTarget = true;
      shortestDistToPose = distToPose;
      closestPose = &targetPose;
      selectedIndex = i;
    }
  } // for each targetPose
    
  CORETECH_ASSERT(foundTarget);
  CORETECH_ASSERT(closestPose);
      
  return this->GetPlan(path, startPose, *closestPose);
}
    
    
} // namespace Cozmo
} // namespace Anki
