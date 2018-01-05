/**
 * File: pathHelper.h
 *
 * Author: Kevin M. Karol
 * Date:   2016-6-17
 *
 * Description: Functions which make path creation easier using a coordinate system
 *   relative to the robot.
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Cozmo_Basestation_PathHelper_H__
#define __Cozmo_Basestation_PathHelper_H__

#include "coretech/common/engine/math/pose.h"
#include "coretech/common/shared/types.h"
#include <vector>

namespace Anki{
//forward declarations
class Pose3d;
namespace Planning{
class Path;

class IRelativePathSegment
{
public:
  virtual Pose3d ExtendPath(const Pose3d& pose, Planning::Path& path) const = 0;
};
  
class RelativePathLine : public IRelativePathSegment
{
public:
  RelativePathLine(f32 distanceForward, f32 targetSpeed, f32 accel, f32 decel);
  virtual Pose3d ExtendPath(const Pose3d& pose, Planning::Path& path) const override;
  
private:
  f32 _distanceForward;
  f32 _targetSpeed;
  f32 _accel;
  f32 _decel;
};

class RelativePathArc : public IRelativePathSegment
{
public:
  RelativePathArc(f32 radius, f32 sweepRad, f32 targetSpeed, f32 accel, f32 decel, bool clockwise);
  virtual Pose3d ExtendPath(const Pose3d& pose, Planning::Path& path) const override;
  
private:
  f32 _radius;
  f32 _sweepRad;
  f32 _targetSpeed;
  f32 _accel;
  f32 _decel;
  bool _clockwise;
};
  
class RelativePathTurn : public IRelativePathSegment
{
public:
  RelativePathTurn(f32 rotRadians, f32 targetRotSpeed, f32 rotAccel, f32 rotDecel, f32 angleTolerance, bool clockwise);
  virtual Pose3d ExtendPath(const Pose3d& pose, Planning::Path& path) const override;
  
private:
  f32 _rotRadians;
  f32 _targetRotSpeed;
  f32 _rotAccel;
  f32 _rotDecel;
  f32 _angleTolerance;
  bool _clockwise;
};
  
  
///////
//// Path Relative Cozmo class
///////


class PathRelativeCozmo{
public:
  //Append all segment data types
  void AppendSegment(std::shared_ptr<const IRelativePathSegment> segment);
  void GetPlanningPath(const Pose3d& startingPose, Planning::Path& path) const;
  
private:
  std::vector<std::shared_ptr<const IRelativePathSegment>> _pathComponents;
  
};// class PathHelper

}// namespace Planning
}// namespace Anki


#endif // __Cozmo_Basestation_PathHelper_H__
