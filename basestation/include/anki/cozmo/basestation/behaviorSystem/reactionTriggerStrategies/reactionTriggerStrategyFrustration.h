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

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorListenerInterfaces/iSubtaskListener.h"
#include "anki/common/basestation/jsonTools.h"

namespace Anki {
namespace Cozmo {

class ReactionTriggerStrategyFrustration : public IReactionTriggerStrategy, public ISubtaskListener{
public:
  ReactionTriggerStrategyFrustration(Robot& robot, const Json::Value& config);
  
  virtual bool ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior) override;
  virtual bool ShouldResumeLastBehavior() const override { return false;}

  virtual bool CanTriggerWhileTriggeredBehaviorRunning() const override { return false; }
  virtual void BehaviorThatStartegyWillTrigger(IBehavior* behavior) override;
  virtual void AnimationComplete() override;
  
private:
  void LoadJson(const Json::Value& config);
  
  float _maxConfidentScore = 0.0f;
  float _cooldownTime_s = 0.0f;
  float _lastReactedTime_s = -1.0f;
};


} // namespace Cozmo
} // namespace Anki

#endif //
