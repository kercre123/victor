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

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"

namespace Anki {
namespace Cozmo {

class ReactionTriggerStrategyPlacedOnCharger : public IReactionTriggerStrategy{
public:
  ReactionTriggerStrategyPlacedOnCharger(Robot& robot, const Json::Value& config);

  virtual bool ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior) override;
  virtual bool ShouldResumeLastBehavior() const override { return false;}  
  virtual bool CanTriggerWhileTriggeredBehaviorRunning() const override { return true; }

protected:
  virtual void AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot) override;
  
  virtual void EnabledStateChanged(bool enabled) override
                 {_shouldComputationallySwitch = false;}
  
private:
  bool _shouldComputationallySwitch = false;
  
  // prevent Cozmo from asking to go to sleep for a period of time after connection
  mutable float _dontRunUntilTime_sec;

};

} // namespace Cozmo
} // namespace Anki

#endif //
