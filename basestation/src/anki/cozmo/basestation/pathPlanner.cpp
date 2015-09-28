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
#include "pathPlanner.h"
#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {


IPathPlanner::IPathPlanner()
  : _hasValidPath(false)
  , _planningError(false)
  , _selectedTargetIdx(0)
{
}

EComputePathStatus IPathPlanner::ComputePath(const Pose3d& startPose,
                                             const std::vector<Pose3d>& targetPoses)
{
  _hasValidPath = false;
  _planningError = false;

  // Select the closest
  const Pose3d* closestPose = nullptr;
      
  _selectedTargetIdx = 0;
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
      _selectedTargetIdx = i;
    }
  } // for each targetPose
    
  CORETECH_ASSERT(foundTarget);
  CORETECH_ASSERT(closestPose);
      
  return ComputePath(startPose, targetPoses[_selectedTargetIdx]);
}

EComputePathStatus IPathPlanner::ComputeNewPathIfNeeded(const Pose3d& startPose,
                                                                      bool forceReplanFromScratch)
{
  return EComputePathStatus::NoPlanNeeded;
}

EPlannerStatus IPathPlanner::CheckPlanningStatus() const
{
  if( _planningError ) {
    return EPlannerStatus::Error;
  }

  if( _hasValidPath ) {
    return EPlannerStatus::CompleteWithPlan;
  }
  else {
    return EPlannerStatus::CompleteNoPlan;
  }
}

bool IPathPlanner::GetCompletePath(Planning::Path &path)
{
  if( ! _hasValidPath ) {
    return false;
  }

  path = _path;
  return true;
}

bool IPathPlanner::GetCompletePath(Planning::Path &path, size_t& selectedTargetIndex)
{
  if( ! _hasValidPath ) {
    return false;
  }

  path = _path;
  selectedTargetIndex = _selectedTargetIdx;
  return true;
}

    
    
} // namespace Cozmo
} // namespace Anki
