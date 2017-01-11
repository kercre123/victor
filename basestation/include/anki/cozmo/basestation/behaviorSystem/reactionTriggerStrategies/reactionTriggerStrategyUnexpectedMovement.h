/**
 * File: reactionTriggerStrategyUnexpectedMovement.h
 *
 * Author: Kevin M. Karol
 * Created: 12/08/16
 *
 * Description: Reaction Trigger strategy for responding to cozmo not moving as much as he expects to
 *
 * Copyright: Anki, Inc. 2016
 *
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyUnexpectedMovement_H__
#define __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyUnexpectedMovement_H__

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"

namespace Anki {
namespace Cozmo {

class ReactionTriggerStrategyUnexpectedMovement : public IReactionTriggerStrategy{
public:
  ReactionTriggerStrategyUnexpectedMovement(Robot& robot, const Json::Value& config);
  
  virtual bool ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior) override;
  virtual bool ShouldResumeLastBehavior() const override { return true;}
  virtual bool CanTriggerWhileTriggeredBehaviorRunning() const override { return true; }
  virtual void AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot) override;
  
  virtual void EnabledStateChanged(bool enabled) override
                 {_shouldComputationallySwitch = false;}
  
private:
  bool _shouldComputationallySwitch = false;
};


} // namespace Cozmo
} // namespace Anki

#endif //
