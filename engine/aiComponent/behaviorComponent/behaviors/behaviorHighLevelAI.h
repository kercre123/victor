/**
 * File: behaviorHighLevelAI.h
 *
 * Author: Brad Neuman
 * Created: 2017-10-16
 *
 * Description: Root behavior to handle the state machine for the high level AI of victor (similar to Cozmo's
 *              freeplay activities)
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorHighLevelAI_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorHighLevelAI_H__

#include "engine/aiComponent/behaviorComponent/behaviors/internalStatesBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorHighLevelAI : public InternalStatesBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorHighLevelAI(const Json::Value& config);
  
public:

  virtual ~BehaviorHighLevelAI();
  
  virtual bool WantsToBeActivatedBehavior() const override { return true; }

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override{
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.wantsToBeActivatedWhenOnCharger = true;
    modifiers.behaviorAlwaysDelegates = false;
  }
  
  virtual void BehaviorUpdate() override;
  
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

private:
  
  bool IsBehaviorActive( BehaviorID behaviorID ) const;
  
  struct {
    float socializeKnownFaceCooldown_s;
    float playWithCubeCooldown_s;
    float playWithCubeOnChargerCooldown_s;
    float goToSleepTimeout_s;
    float minFaceTimeToAllowSleep_s;
    float maxFaceDistanceToSocialize_mm;
  } _params;
  
  PreDefinedStrategiesMap CreatePreDefinedStrategies();
  
};

}
}

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorHighLevelAI_H__
