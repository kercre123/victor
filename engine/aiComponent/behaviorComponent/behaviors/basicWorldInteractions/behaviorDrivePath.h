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

#include "coretech/common/engine/math/pose.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "coretech/planning/shared/path.h"

namespace Anki {
//forward declaration
class Pose3d;
  
namespace Cozmo {
  
class BehaviorDrivePath : public ICozmoBehavior
{
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorDrivePath(const Json::Value& config);
  
public:  
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {}

  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  
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
  void TransitionToFollowingPath(BehaviorExternalInterface& behaviorExternalInterface);
  
}; //class BehaviorDrivePath

}//namespace Cozmo
}//namespace Anki


#endif // __Cozmo_Basestation_Behaviors_BehaviorDrivePath_H__
