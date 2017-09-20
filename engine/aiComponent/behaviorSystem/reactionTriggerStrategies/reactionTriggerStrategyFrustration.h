/**
 * File: reactionTriggerStrategyFrustration.h
 *
 * Author: Kevin M. Karol
 * Created: 12/08/16
 *
 * Description: Reaction Trigger strategy for responding to frustration mood
 *
 * Copyright: Anki, Inc. 2016
 *
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyFrustration_H__
#define __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyFrustration_H__

#include "engine/aiComponent/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"
#include "engine/aiComponent/behaviorSystem/behaviorListenerInterfaces/iSubtaskListener.h"
#include "anki/common/basestation/jsonTools.h"

namespace Anki {
namespace Cozmo {

class ReactionTriggerStrategyFrustration : public IReactionTriggerStrategy, public ISubtaskListener{
public:
  ReactionTriggerStrategyFrustration(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config);
  
  virtual bool ShouldResumeLastBehavior() const override { return false;}

  virtual bool CanInterruptOtherTriggeredBehavior() const override { return false; }
  virtual void AnimationComplete(BehaviorExternalInterface& behaviorExternalInterface) override;

protected:
  virtual void BehaviorThatStrategyWillTriggerInternal(IBehaviorPtr behavior) override;
  virtual bool ShouldTriggerBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr behavior) override;
  virtual void SetupForceTriggerBehavior(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr behavior) override;

private:
  void LoadJson(const Json::Value& config);
  
  float _maxConfidentScore = 0.0f;
  float _cooldownTime_s = 0.0f;
  float _lastReactedTime_s = -1.0f;
};


} // namespace Cozmo
} // namespace Anki

#endif //
