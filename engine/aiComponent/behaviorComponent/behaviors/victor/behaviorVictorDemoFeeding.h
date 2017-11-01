/**
 * File: behaviorVictorDemoFeeding.h
 *
 * Author: Brad Neuman
 * Created: 2017-10-12
 *
 * Description: Observing demo version of feeding. This is a replacement for the old feeding activity
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_Victor_BehaviorVictorDemoFeeding_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_Victor_BehaviorVictorDemoFeeding_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "anki/common/basestation/objectIDs.h"

namespace Anki {
namespace Cozmo {

class BehaviorFeedingEat;

class BehaviorVictorDemoFeeding : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;  
  BehaviorVictorDemoFeeding(const Json::Value& config);

public:

  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}

protected:

  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;

  virtual void InitBehavior(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;

private:

  void TransitionToVerifyFood(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToWaitForFood(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToEating(BehaviorExternalInterface& behaviorExternalInterface);

  TimeStamp_t _imgTimeStartedWaitngForFood = 0;

  std::shared_ptr<BehaviorFeedingEat> _eatFoodBehavior;
};

}
}

#endif
