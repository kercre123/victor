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

  virtual bool ShouldResumeLastBehavior() const override { return false;}  
  virtual bool CanInterruptOtherTriggeredBehavior() const override { return true; }

protected:
  virtual void AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot) override;
  virtual bool ShouldTriggerBehaviorInternal(const Robot& robot, const IBehavior* behavior) override;
  virtual void SetupForceTriggerBehavior(const Robot& robot, const IBehavior* behavior) override;

  virtual void EnabledStateChanged(bool enabled) override
                 {_shouldTrigger = false;}
  
private:
  bool _shouldTrigger = false;
  
  // prevent Cozmo from asking to go to sleep for a period of time after connection
  mutable float _dontRunUntilTime_sec;

};

} // namespace Cozmo
} // namespace Anki

#endif //
