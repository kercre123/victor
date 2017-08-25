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

#include "engine/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"

namespace Anki {
namespace Cozmo {

class ReactionTriggerStrategyRobotShaken : public IReactionTriggerStrategy
{
public:
  ReactionTriggerStrategyRobotShaken(Robot& robot, const Json::Value& config);
  
  virtual bool ShouldResumeLastBehavior() const override { return false;}
  virtual bool CanInterruptOtherTriggeredBehavior() const override { return true; }

protected:
  virtual bool ShouldTriggerBehaviorInternal(const Robot& robot, const IBehaviorPtr behavior) override;
  virtual void SetupForceTriggerBehavior(const Robot& robot, const IBehaviorPtr behavior) override;

};


} // namespace Cozmo
} // namespace Anki

#endif //
