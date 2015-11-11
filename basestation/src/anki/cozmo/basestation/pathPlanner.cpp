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

bool IPathPlanner::GetCompletePath(const Pose3d& currentRobotPose,
                                   Planning::Path &path,
                                   const PathMotionProfile* motionProfile)
{
  if (GetCompletePath_Internal(currentRobotPose, path)) {
    
    if (motionProfile != nullptr) {
      ApplyMotionProfile(path, *motionProfile, _path);
    }
    path = _path;
    return true;
    
  }
  
  return false;
}
  
bool IPathPlanner::GetCompletePath(const Pose3d& currentRobotPose,
                                   Planning::Path &path,
                                   size_t& selectedTargetIndex,
                                   const PathMotionProfile* motionProfile)
{
  if (GetCompletePath_Internal(currentRobotPose, path, selectedTargetIndex)) {
    
    if (motionProfile != nullptr) {
      ApplyMotionProfile(path, *motionProfile, _path);
    }
    path = _path;
    return true;
    
  }
  
  return false;
}

  
bool IPathPlanner::GetCompletePath_Internal(const Pose3d& currentRobotPose,
                                            Planning::Path &path)
{
  if( ! _hasValidPath ) {
    return false;
  }

  return true;
}

bool IPathPlanner::GetCompletePath_Internal(const Pose3d& currentRobotPose,
                                            Planning::Path &path,
                                            size_t& selectedTargetIndex)
{
  if( ! _hasValidPath ) {
    return false;
  }
  
  selectedTargetIndex = _selectedTargetIdx;
  return true;
}


bool IPathPlanner::ApplyMotionProfile(const Planning::Path &in,
                                      const PathMotionProfile& motionProfile,
                                      Planning::Path &out) const
{
  out.Clear();

  const f32 lin_speed = fabsf(motionProfile.speed_mmps);
  const f32 arc_speed = 0.75f * lin_speed;  // TEMP: We should actually use max wheel speeds to compute linear speed along arc.
  const f32 turn_speed = fabsf(motionProfile.pointTurnSpeed_rad_per_sec);
  
  for (int i=0; i < in.GetNumSegments(); ++i)
  {
    f32 speed = lin_speed;
    
    Planning::PathSegment seg = in.GetSegmentConstRef(i);
    switch(seg.GetType()) {
      case Planning::PST_ARC:
        speed = arc_speed;
      case Planning::PST_LINE:
      {
        seg.SetSpeedProfile(seg.GetTargetSpeed() > 0 ? speed : -speed,
                            motionProfile.accel_mmps2,
                            motionProfile.decel_mmps2);
        break;
      }
      case Planning::PST_POINT_TURN:
      {
        seg.SetSpeedProfile(seg.GetTargetSpeed() > 0 ? turn_speed : -turn_speed,
                            motionProfile.pointTurnAccel_rad_per_sec2,
                            motionProfile.pointTurnDecel_rad_per_sec2);
        break;
      }
      default:
        PRINT_NAMED_WARNING("IPathPlanner.ApplyMotionProfile.UnknownSegment", "Path has invalid segment");
        return false;
    }
    out.AppendSegment(seg);
  }
  
  return true;
}
  
    
} // namespace Cozmo
} // namespace Anki
