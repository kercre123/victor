/**
 *  File: BehaviorBuildPyramid.hpp
 *  cozmoEngine
 *
 *  Author: Kevin M. Karol
 *  Created: 2016-08-09
 *
 *  Description: Behavior which allows cozmo to build a pyramid from 3 blocks
 *
 *  Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorBuildPyramid_H__
#define __Cozmo_Basestation_Behaviors_BehaviorBuildPyramid_H__

#include "coretech/common/engine/objectIDs.h"
#include "coretech/common/engine/math/pose.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/buildPyramid/behaviorBuildPyramidBase.h"

namespace Anki {
//forward declaration
class Pose3d;
namespace Cozmo {
class ObservableObject;
enum class AnimationTrigger;
  
class BehaviorBuildPyramid : public BehaviorBuildPyramidBase
{
protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorBuildPyramid(const Json::Value& config);
  
public:
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;  

protected:
  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  
private:
  typedef std::vector<const ObservableObject*> BlockList;
  
  void TransitionToDrivingToTopBlock(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToPlacingTopBlock(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToReactingToPyramid(BehaviorExternalInterface& behaviorExternalInterface);
        
}; //class BehaviorBuildPyramid

}//namespace Cozmo
}//namespace Anki


#endif // __Cozmo_Basestation_Behaviors_BehaviorBuildPyramid_H__
