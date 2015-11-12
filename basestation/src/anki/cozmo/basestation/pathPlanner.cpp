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


IPathPlanner::IPathPlanner()
  : _hasValidPath(false)
  , _planningError(false)
  , _selectedTargetIdx(0)
{
}

size_t IPathPlanner::ComputeClosestGoalPose(const Pose3d& startPose,
                                            const std::vector<Pose3d>& targetPoses)
{
  size_t selectedTargetIdx = 0;
  bool foundTarget = false;
  f32 shortestDistToPose = -1.f;
  for(size_t i=0; i<targetPoses.size(); ++i)
  {
    const Pose3d& targetPose = targetPoses[i];
        
    const f32 distToPose = (targetPose.GetTranslation() - startPose.GetTranslation()).LengthSq();
    if (!foundTarget || distToPose < shortestDistToPose)
    {
      foundTarget = true;
      shortestDistToPose = distToPose;
      selectedTargetIdx = i;
    }

    PRINT_NAMED_DEBUG("IPathPlanner.ComputeClosestGoalPose",
                      "Candidate target pose: (%.2f %.2f %.2f), %.1fdeg @ (%.2f %.2f %.2f): dist %f",
                      targetPose.GetTranslation().x(),
                      targetPose.GetTranslation().y(),
                      targetPose.GetTranslation().z(),
                      targetPose.GetRotationAngle<'Z'>().getDegrees(),
                      targetPose.GetRotationAxis().x(),
                      targetPose.GetRotationAxis().y(),
                      targetPose.GetRotationAxis().z(),
                      distToPose);
  }

  return selectedTargetIdx;
}

EComputePathStatus IPathPlanner::ComputePath(const Pose3d& startPose,
                                             const std::vector<Pose3d>& targetPoses)
{
  _hasValidPath = false;
  _planningError = false;

  // Select the closest
  _selectedTargetIdx = ComputeClosestGoalPose(startPose, targetPoses);
      
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

bool IPathPlanner::GetCompletePath(const Pose3d& currentRobotPose, Planning::Path &path)
{
  if( ! _hasValidPath ) {
    return false;
  }

  path = _path;
  return true;
}

bool IPathPlanner::GetCompletePath(const Pose3d& currentRobotPose, Planning::Path &path, size_t& selectedTargetIndex)
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
