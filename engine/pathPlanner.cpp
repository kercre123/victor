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

#include "coretech/common/engine/math/pose.h"
#include "coretech/common/engine/math/point_impl.h"
#include "coretech/planning/engine/robotActionParams.h"
#include "pathPlanner.h"
#include "util/logging/logging.h"
#include "util/math/math.h"


namespace Anki {
namespace Cozmo {


IPathPlanner::IPathPlanner(const std::string& name)
  : _hasValidPath(false)
  , _planningError(false)
  , _selectedTargetIdx(0)
  , _name(name)
{
}

Planning::GoalID IPathPlanner::ComputeClosestGoalPose(const Pose3d& startPose,
                                                      const std::vector<Pose3d>& targetPoses)
{
  Planning::GoalID selectedTargetIdx = 0;
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
                                                        bool forceReplanFromScratch,
                                                        bool allowGoalChange)
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
  
EPlannerErrorType IPathPlanner::GetErrorType() const
{
  DEV_ASSERT( CheckPlanningStatus() == EPlannerStatus::Error, "IPathPlanner.GetErrorType.NoError" );
  return EPlannerErrorType::PlannerFailed;
}

bool IPathPlanner::GetCompletePath(const Pose3d& currentRobotPose,
                                   Planning::Path &path,
                                   const PathMotionProfile* motionProfile)
{
  if (GetCompletePath_Internal(currentRobotPose, path)) {
    
    if (motionProfile != nullptr) {
      ApplyMotionProfile(path, *motionProfile, _path);
      path = _path;
    } else {
      _path = path;
    }
    return true;
    
  }
  
  return false;
}
  
bool IPathPlanner::GetCompletePath(const Pose3d& currentRobotPose,
                                   Planning::Path &path,
                                   Planning::GoalID& selectedTargetIndex,
                                   const PathMotionProfile* motionProfile)
{
  if (GetCompletePath_Internal(currentRobotPose, path, selectedTargetIndex)) {
    
    if (motionProfile != nullptr) {
      ApplyMotionProfile(path, *motionProfile, _path);
      path = _path;
    } else {
      _path = path;
    }
    return true;
    
  }
  
  return false;
}

bool IPathPlanner::CheckIsPathSafe(const Planning::Path& path, float startAngle) const
{
  Planning::Path waste;
  return CheckIsPathSafe(path, startAngle, waste);
}

bool IPathPlanner::CheckIsPathSafe(const Planning::Path& path, float startAngle, Planning::Path& validPath) const
{
  validPath = path;
  return true;
}

  
bool IPathPlanner::GetCompletePath_Internal(const Pose3d& currentRobotPose,
                                            Planning::Path &path)
{
  if( ! _hasValidPath ) {
    return false;
  }
  
  path = _path;
  return true;
}

bool IPathPlanner::GetCompletePath_Internal(const Pose3d& currentRobotPose,
                                            Planning::Path &path,
                                            Planning::GoalID& selectedTargetIndex)
{
  if( ! _hasValidPath ) {
    return false;
  }
  
  path = _path;
  selectedTargetIndex = _selectedTargetIdx;
  return true;
}


bool IPathPlanner::ApplyMotionProfile(const Planning::Path &in,
                                      const PathMotionProfile& motionProfile,
                                      Planning::Path &out)
{
  out.Clear();
  std::vector<Planning::PathSegment> reversedPath;
  
  const f32 lin_speed = fabsf(motionProfile.speed_mmps);
  const f32 turn_speed = fabsf(motionProfile.pointTurnSpeed_rad_per_sec);
  
  // Create RobotActionParam which holds max wheel speeds and other parameters necessary to compute
  // valid motion profile on path.
  Planning::RobotActionParams actionParams;
  bool nextSegEndsInStop = true;
  
  // Figure out proper path segment speeds to account for deceleration starting from the end of the path and working
  // towards the start since we know the last segment will end in a stop, this loop calculates each segments neccessary initial speed.
  // After this loop, reversedPath will contain a path which is backwards and the segment speeds are off by one segment (initial vs final speed)
  for (int i=in.GetNumSegments()-1; i >= 0; --i)
  {
    Planning::PathSegment seg = in.GetSegmentConstRef(i);
    int speedSign = std::copysign(1, seg.GetTargetSpeed()); //Keep track of the direction of this segment's speed
    
    // If this isn't a point turn prepopulate it's speed, accel, and decel according to the motionProfile which are used
    // to calculate appropriate speeds
    if(seg.GetType() != Planning::PST_POINT_TURN)
    {
      seg.SetSpeedProfile(motionProfile.speed_mmps * speedSign, motionProfile.accel_mmps2, motionProfile.decel_mmps2);
    }
    
    // If the segment before the current segment is a point turn the intial speed entering the current segment will
    // be zero
    bool prevSegIsPT = false;
    if(i-1 >= 0)
    {
      prevSegIsPT = in.GetSegmentConstRef(i-1).GetType() == Planning::PST_POINT_TURN;
    }
    
    // Limit linear speed based on direction-dependent max wheel speed
    f32 speed = lin_speed;
    if (seg.GetTargetSpeed() < 0) {
      const f32 absSpeed = fabsf(motionProfile.reverseSpeed_mmps);
      if (FLT_GT(absSpeed, 0.0f))
      {
        speed = absSpeed;
      }
      else{
        PRINT_NAMED_WARNING("IPathPlanner.ApplyMotionProfile", "Tried to set speed to 0! PathMotionProfile.reverseSpeed_mmps = 0! Using speed_mmps instead.");
      }
    }
    
    switch(seg.GetType()) {
      case Planning::PST_ARC:
      {
        // Scale speed along arc to accommodate the max wheel speed
        f32 arcRadius_mm = std::fabsf(seg.GetDef().arc.radius);
        speed = (arcRadius_mm * speed) / (arcRadius_mm + actionParams.halfWheelBase_mm);
        // fall through to PST_LINE handling
      }
      case Planning::PST_LINE:
      {
        f32 finalSpeed = 0;
        // If the segment after the current segment does not end in a stop (point turn or end of path) this segment's final speed
        // will be the target speed of the segment after us (since we are traversing backwards we will have just processed the
        // segment after this one)
        if(!nextSegEndsInStop)
        {
          finalSpeed = reversedPath[reversedPath.size()-1].GetTargetSpeed();
        }
        
        f32 initialSpeed = 0;
        // If there is only one segment then our initial speed can't be zero (although it is) because then our initial and final
        // final speeds will be zero causing this segment to have a deceleration of zero
        if(in.GetNumSegments() == 1)
        {
          initialSpeed = motionProfile.speed_mmps;
        }
        // Otherwise this segment is not the first segment of the path and it isn't a point turn
        else if(i > 0 && !prevSegIsPT)
        {
          initialSpeed = motionProfile.speed_mmps;
        }
        
        // Calculate the actual deceleration neccessary to slow down over this segment
        f32 actualSegDecel = ((finalSpeed*finalSpeed)-(initialSpeed*initialSpeed)) / (-2*seg.GetLength());
        
        // If this segment has no deceleration (maintaining the same speed throughout)
        if(NEAR_ZERO(actualSegDecel))
        {
          nextSegEndsInStop = false;
          seg.SetSpeedProfile(std::copysign(seg.GetTargetSpeed(), speedSign),
                              motionProfile.accel_mmps2,
                              motionProfile.decel_mmps2);
          break;
        }
        
        // If the actual deceleration neccessary to slow down over this segment is greater than our desired deceleration
        // calculate a new final speed for this segment (target speed)
        // And the difference between the two decelerations is greater than 1, this is to prevent potentially splitting
        // the segment into two segments where one of them has a near zero length
        if(actualSegDecel > motionProfile.decel_mmps2 && ABS(actualSegDecel - motionProfile.decel_mmps2) > diffInDecel)
        {
          // Our new target speed will be the speed reached if applying the deceleration over the entire segment
          f32 newTargetSpeed = sqrtf(-(motionProfile.decel_mmps2*(-2*seg.GetLength()) - finalSpeed*finalSpeed));
          seg.SetTargetSpeed(std::copysign(newTargetSpeed, speedSign));
        }
      
        // Update our speedProfile and the next segment that will be processed is not going to end in a stop because this is a line
        seg.SetSpeedProfile(seg.GetTargetSpeed(),
                            motionProfile.accel_mmps2,
                            motionProfile.decel_mmps2);
        nextSegEndsInStop = false;
        break;
      }
      case Planning::PST_POINT_TURN:
      {
        // Next segment to be processed will end in a stop since point turns require the robot to be stopped
        nextSegEndsInStop = true;
        seg.SetSpeedProfile(std::copysign(turn_speed, seg.GetTargetSpeed()),
                            motionProfile.pointTurnAccel_rad_per_sec2,
                            motionProfile.pointTurnDecel_rad_per_sec2);
        break;
      }
      default:
        PRINT_NAMED_WARNING("IPathPlanner.ApplyMotionProfile.UnknownSegment", "Path has invalid segment");
        return false;
    }
    
    reversedPath.push_back(seg);
  }
  
  int numSegs = (int)reversedPath.size();
  
  // Because the deceleration related calculations needed to be made while traversing the path backwards to know when we should start
  // decelerating we need to go back through the path, forwards this time, and shift target speeds over one segment and decide if
  // we can split any of the line segments into two segments
  // After this loop, out will contain the final path with the correct speeds in order to produce a path with smooth deceleration
  for(int i = numSegs-1; i >= 0; i--)
  {
    Planning::PathSegment seg = reversedPath[i];
    
    bool hasNextSeg = (i-1 >= 0);
    Planning::PathSegment nextSeg;
    if(hasNextSeg)
    {
      nextSeg = reversedPath[i-1];
    }
    
    // This segment's speed is actually the next segment's speed (intial speed vs final speed) unless this is the
    // last segment (has no next)
    f32 speed = (hasNextSeg ? nextSeg.GetTargetSpeed() : 0);
    
    if(hasNextSeg)
    {
      // If the next segment is a point turn
      if(nextSeg.GetType() == Planning::PST_POINT_TURN)
      {
        speed = 0;
      }
      else if((seg.GetTargetSpeed() > 0) != (nextSeg.GetTargetSpeed() > 0))
      {
        // If the segment after this one is not a point turn and the sign of it's speed is different
        // than the sign of the current segment's speed (segments go opposite directions)
        // then make sure we preserve the sign of the current segment's speed
        speed = std::copysign(speed, seg.GetTargetSpeed());
      }
    }
    
    // If this is a line segment check if we can split it into two segments because we can finish decelerating before reaching
    // the end of the segment
    if(seg.GetType() == Planning::PST_LINE)
    {
      f32 initialSpeed = seg.GetTargetSpeed();
      
      f32 distToDecel = ((speed*speed)-(initialSpeed*initialSpeed)) / (-2*seg.GetDecel());
      
      // Only split the segment if the distance it takes to decelerate is not within 5mm of the length of the segment and
      // it is smaller than the length of the segment
      if(distToDecel > 0 &&
         distToDecel < seg.GetLength() &&
         !NEAR(distToDecel, seg.GetLength(), distToDecelSegLenTolerance_mm))
      {
        Planning::PathSegment newSeg;
        f32 startX, startY, endX, endY, endA;
        seg.GetStartPoint(startX, startY);
        seg.GetEndPose(endX, endY, endA);
        f32 newSegLen = std::copysign(seg.GetLength() - distToDecel, initialSpeed);
        newSeg.DefineLine(startX,
                          startY,
                          startX+(newSegLen*cosf(endA)),
                          startY+(newSegLen*sinf(endA)),
                          initialSpeed,
                          seg.GetAccel(),
                          seg.GetDecel());
        
        // If the second half of this segment will end in a stop set its speed to finalPathSegmentSpeed to prevent us from
        // prematurely stopping while following the path due to unknown forces
        seg.DefineLine(startX+(newSegLen*cosf(endA)),
                       startY+(newSegLen*sinf(endA)),
                       endX,
                       endY,
                       (speed == 0 ? std::copysign(finalPathSegmentSpeed_mmps,initialSpeed) : std::copysign(speed, initialSpeed)),
                       seg.GetAccel(),
                       seg.GetDecel());
        
        out.AppendSegment(newSeg);
        out.AppendSegment(seg);
        continue;
      }
    }
    
    // If this segments speed is too low or it is the last segment set its speed to finalPathSegmentSpeed (slowest we can go)
    if(std::abs(speed) < finalPathSegmentSpeed_mmps || !hasNextSeg)
    {
      speed = std::copysign(finalPathSegmentSpeed_mmps,speed);
    }
    
    Planning::PathSegmentType prevSegType = Planning::PST_UNKNOWN;
    if(i+1 < numSegs)
    {
      prevSegType = reversedPath[i+1].GetType();
    }
    
    // If this is the last segment and the segment before this is a point turn
    bool lastSegmentAndPrevIsPT = (!hasNextSeg &&
                                   prevSegType == Planning::PST_POINT_TURN);
    
    // Only update this segment's speed if it isn't the only segment, it isn't a point turn, the next segment isn't a point turn,
    // and this isn't a special case where this is the last segment and the previous segment is a point turn
    if(numSegs > 1 &&
       seg.GetType() != Planning::PST_POINT_TURN &&
       (hasNextSeg ? nextSeg.GetType() != Planning::PST_POINT_TURN : true) &&
       !lastSegmentAndPrevIsPT)
    {
      seg.SetTargetSpeed(speed);
    }

    out.AppendSegment(seg);
  }
  
  out.PrintPath();
  
  return true;
}


} // namespace Cozmo
} // namespace Anki
