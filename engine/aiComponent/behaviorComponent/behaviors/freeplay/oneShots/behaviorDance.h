/**
 * File: behaviorDance.h
 *
 * Author: Al Chaussee
 * Created: 05/11/17
 *
 * Description: Behavior to have Cozmo dance
 *              Plays dancing animation, triggers music from device, and 
 *              plays cube light animations
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorPlayAnimSequence.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorDance : public BehaviorPlayAnimSequence
{
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorDance(const Json::Value& config);
  
protected:

  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)   override;
  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)   override;
  
private:
  
  // Callback function used to pick a new random cube animation trigger
  // when the previous one completes
  void CubeAnimComplete(BehaviorExternalInterface& behaviorExternalInterface, const ObjectID& objectID);
  
  // Returns a random dancing related cube animation trigger that is not currently being played
  CubeAnimationTrigger GetRandomAnimTrigger(BehaviorExternalInterface& behaviorExternalInterface, const CubeAnimationTrigger& prevTrigger) const;
  
  // Map to store the last CubeAnimationTrigger that was played
  std::map<ObjectID, CubeAnimationTrigger> _lastAnimTrigger;
};
  
}
}
