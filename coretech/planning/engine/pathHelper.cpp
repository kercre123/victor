/**
 * File: pathHelper.cpp
 *
 * Author: Kevin M. Karol
 * Date:   2016-6-17
 *
 * Description: Functions which make path creation easier using a coordinate system
 *   relative to the robot.
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "coretech/planning/engine/pathHelper.h"
#include "coretech/common/engine/math/pose.h"
#include "coretech/planning/shared/path.h"
#include "coretech/common/engine/math/pose.h"
 

namespace Anki{
namespace Planning{

//Append all segment data types
void PathRelativeCozmo::AppendSegment(std::shared_ptr<const IRelativePathSegment> segment){
  _pathComponents.push_back(segment);
};
  
void PathRelativeCozmo::GetPlanningPath(const Pose3d& startingPose, Planning::Path& path) const
{
  Pose3d lastPose = startingPose;
  
  for(auto segment: _pathComponents){
    lastPose = segment->ExtendPath(lastPose, path);
  }
}
 
  
//////////
////Line Implementation
//////////
RelativePathLine::RelativePathLine(f32 distanceForward, f32 targetSpeed, f32 accel, f32 decel)
  :_distanceForward(distanceForward)
  ,_targetSpeed(targetSpeed)
  ,_accel(accel)
  ,_decel(decel)
{}
  
Pose3d RelativePathLine::ExtendPath(const Pose3d& pose, Planning::Path& path) const
{
  Vec3f trans = pose.GetTranslation();
  //Get append values
  f32 x_start = trans.x();
  f32 y_start = trans.y();
  
  //Calculate robot end position
  Pose3d endPosition = Pose3d(0, Z_AXIS_3D(), {_distanceForward, 0.f,0.f}, pose);
  Pose3d endPositionOrigin = endPosition.GetWithRespectToRoot();
  f32 x_end = endPositionOrigin.GetTranslation().x();
  f32 y_end = endPositionOrigin.GetTranslation().y();
  
  path.AppendLine(x_start, y_start, x_end, y_end, _targetSpeed, _accel, _decel);
  
  //Create Ending Pose
  return endPositionOrigin;
}

  
//////////
////Arc Implementation
//////////

RelativePathArc::RelativePathArc(f32 radius, f32 sweepRad, f32 targetSpeed, f32 accel, f32 decel, bool clockwise)
  :_radius(radius)
  ,_sweepRad(sweepRad)
  ,_targetSpeed(targetSpeed)
  ,_accel(accel)
  ,_decel(decel)
  ,_clockwise(clockwise)
{}

  
Pose3d RelativePathArc::ExtendPath(const Pose3d& pose, Planning::Path& path) const
{
  //get append values
  Radians startAngle = pose.GetRotation().GetAngleAroundZaxis();
  f32 radius = _radius;
  f32 sweep = _sweepRad;
  
  //calculate arc center
  if(_clockwise){
    radius = -radius;
    sweep = -sweep;
    startAngle += M_PI_2;
  }else{
    startAngle -= M_PI_2;
  }
    
  Pose3d arcCenterRobot = Pose3d(0, Z_AXIS_3D(), {0.f, radius, 0.f}, pose);
  Pose3d arcCenterOrigin = arcCenterRobot.GetWithRespectToRoot();
  f32 x_center = arcCenterOrigin.GetTranslation().x();
  f32 y_center = arcCenterOrigin.GetTranslation().y();

  path.AppendArc(x_center, y_center, fabs(radius), startAngle.ToFloat(), sweep, _targetSpeed, _accel, _decel);
  
  //Calculate new translation
  Pose3d newRobotRotated = Pose3d(Radians(sweep), Z_AXIS_3D(), {0.f, 0.f, 0.f}, arcCenterRobot);
  Pose3d newRobotTranslated = Pose3d(0, Z_AXIS_3D(), {0.f, -radius, 0.f}, newRobotRotated);
  Pose3d newRobotOrigin = newRobotTranslated.GetWithRespectToRoot();

  //Create Ending Pose
  return newRobotOrigin;
}
  
  
//////////
////Turn Implementation
//////////
  
RelativePathTurn::RelativePathTurn(f32 rotRadians, f32 targetRotSpeed, f32 rotAccel, f32 rotDecel, f32 angleTolerance, bool clockwise)
  :_rotRadians(rotRadians)
  ,_targetRotSpeed(targetRotSpeed)
  ,_rotAccel(rotAccel)
  ,_rotDecel(rotDecel)
  ,_angleTolerance(angleTolerance)
  ,_clockwise(clockwise)
{}

Pose3d RelativePathTurn::ExtendPath(const Pose3d& pose, Planning::Path& path) const
{
  //Get append values
  f32 x_start = pose.GetTranslation().x();
  f32 y_start = pose.GetTranslation().y();
  
  //Calculate target angle
  const Rotation3d& rotation = pose.GetRotation();
  Radians zAxisRotation = rotation.GetAngleAroundZaxis();
  const f32 startAngle = zAxisRotation.ToFloat();
  Radians rotRadians = Radians(_rotRadians);
  
  if(_clockwise){
    rotRadians = -rotRadians;
  }
  
  zAxisRotation += rotRadians;
  
  //Calculate whether this is shortest distance
  bool useShortestDistance = true;
  if(rotRadians.getAbsoluteVal() > M_PI){
    useShortestDistance = false;
  }
  
  path.AppendPointTurn(x_start, y_start, startAngle, zAxisRotation.ToFloat(), _targetRotSpeed, _rotAccel, _rotDecel, _angleTolerance, useShortestDistance);
  
  //Create Ending Pose
  Pose3d newRobotRotated = Pose3d(rotRadians, Z_AXIS_3D(), {0.f, 0.f, 0.f}, pose);
  Pose3d newRobotOrigin = newRobotRotated.GetWithRespectToRoot();
  return newRobotOrigin;
  
}
  
} // namespace Planning
} // namespace Anki
