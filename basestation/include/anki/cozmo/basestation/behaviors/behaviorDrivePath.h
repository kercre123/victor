/**
 *  File: BehaviorDrivePath.hpp
 *  cozmoEngine
 *
 *  Author: Kevin M. Karol
 *  Created: 2016-06-16
 *
 *  Description: Behavior which allows Cozmo to drive around in a cricle
 *
 *  Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorDrivePath_H__
#define __Cozmo_Basestation_Behaviors_BehaviorDrivePath_H__

#include "anki/common/basestation/math/pose.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/planning/shared/path.h"

namespace Anki {
//forward declaration
class Pose3d;
  
namespace Cozmo {
  
class BehaviorDrivePath : public IBehavior
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorDrivePath(Robot& robot, const Json::Value& config);
  
public:
  
  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}

protected:
  
  virtual Result InitInternal(Robot& robot) override;
  virtual float EvaluateScoreInternal(const Robot& robot) const override;
  
  enum class DebugState {
    FollowingPath,
  };
  
  //The path to follow
  Planning::Path _path;
  
  //Building and selecting Path
  virtual void SelectPath(const Pose3d& startingPose, Planning::Path& path);
  virtual void BuildSquare(const Pose3d& startingPose, Planning::Path& path);
  virtual void BuildFigureEight(const Pose3d& startingPose, Planning::Path& path);
  virtual void BuildZ(const Pose3d& startingPose, Planning::Path& path);
  
  
private:
  void TransitionToFollowingPath(Robot& robot);
  
}; //class BehaviorDrivePath

}//namespace Cozmo
}//namespace Anki


#endif // __Cozmo_Basestation_Behaviors_BehaviorDrivePath_H__
