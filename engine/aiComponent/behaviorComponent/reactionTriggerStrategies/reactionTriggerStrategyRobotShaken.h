/**
 * File: reactionTriggerStrategyRobotShaken.h
 *
 * Author: Matt Michini
 * Created: 2017/01/11
 *
 * Description: Reaction Trigger strategy for responding to robot being shaken
 *
 * Copyright: Anki, Inc. 2017
 *
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyRobotShaken_H__
#define __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyRobotShaken_H__

#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/iReactionTriggerStrategy.h"

namespace Anki {
namespace Cozmo {

class ReactionTriggerStrategyRobotShaken : public IReactionTriggerStrategy
{
public:
  ReactionTriggerStrategyRobotShaken(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config);
  
  virtual bool ShouldResumeLastBehavior() const override { return false;}
  virtual bool CanInterruptOtherTriggeredBehavior() const override { return true; }

protected:
  virtual bool ShouldTriggerBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr behavior) override;
  virtual void SetupForceTriggerBehavior(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr behavior) override;

};


} // namespace Cozmo
} // namespace Anki

#endif //
