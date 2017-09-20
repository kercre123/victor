/**
 * File: reactionTriggerStrategySparked.h
 *
 * Author: Kevin M. Karol
 * Created: 12/08/16
 *
 * Description: Reaction Trigger strategy for responding to being sparked
 *
 * Copyright: Anki, Inc. 2016
 *
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategySparked_H__
#define __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategySparked_H__

#include "engine/aiComponent/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"

namespace Anki {
namespace Cozmo {

class ReactionTriggerStrategySparked : public IReactionTriggerStrategy{
public:
  ReactionTriggerStrategySparked(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config);
  
  virtual bool ShouldResumeLastBehavior() const override { return false;}
  virtual bool CanInterruptOtherTriggeredBehavior() const override { return true; }

protected:
  virtual bool ShouldTriggerBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr behavior) override;
  virtual void SetupForceTriggerBehavior(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr behavior) override;

};


} // namespace Cozmo
} // namespace Anki

#endif //
