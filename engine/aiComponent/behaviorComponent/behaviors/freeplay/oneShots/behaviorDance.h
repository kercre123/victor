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

#ifndef __Cozmo_Basestation_Behaviors_BehaviorDance_H__
#define __Cozmo_Basestation_Behaviors_BehaviorDance_H__

#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorAnimSequence.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorDance : public BehaviorAnimSequence
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorDance(const Json::Value& config);
  
protected:

  virtual void OnBehaviorActivated()   override;
  virtual void OnBehaviorDeactivated()   override;
  
private:
  
  // Callback function used to pick a new random cube animation trigger
  // when the previous one completes
  void CubeAnimComplete(const ObjectID& objectID);
  
  // Returns a random dancing related cube animation trigger that is not currently being played
  CubeAnimationTrigger GetRandomAnimTrigger(const CubeAnimationTrigger& prevTrigger) const;
  
  // Map to store the last CubeAnimationTrigger that was played
  std::map<ObjectID, CubeAnimationTrigger> _lastAnimTrigger;
};
  
}
}

#endif //__Cozmo_Basestation_Behaviors_BehaviorDance_H__
