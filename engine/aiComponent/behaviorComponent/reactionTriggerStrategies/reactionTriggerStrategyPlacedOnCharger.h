/**
 * File: reactionTriggerStrategyOnCharger.h
 *
 * Author: Kevin M. Karol
 * Created: 12/08/16
 *
 * Description: Reaction Trigger strategy for responding to being placed on the charger
 *
 * Copyright: Anki, Inc. 2016
 *
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyOnCharger_H__
#define __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyOnCharger_H__

#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/iReactionTriggerStrategy.h"

namespace Anki {
namespace Cozmo {

class ReactionTriggerStrategyPlacedOnCharger : public IReactionTriggerStrategy{
public:
  ReactionTriggerStrategyPlacedOnCharger(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config);

  virtual bool ShouldResumeLastBehavior() const override { return false;}  
  virtual bool CanInterruptOtherTriggeredBehavior() const override { return true; }

protected:
  virtual bool ShouldTriggerBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr behavior) override;
  virtual void SetupForceTriggerBehavior(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr behavior) override;

};

} // namespace Cozmo
} // namespace Anki

#endif //
