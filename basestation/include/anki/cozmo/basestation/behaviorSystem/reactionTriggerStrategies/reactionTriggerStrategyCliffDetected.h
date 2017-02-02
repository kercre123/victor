/**
 * File: reactionTriggerStrategyCliff.h
 *
 * Author: Kevin M. Karol
 * Created: 12/08/16
 *
 * Description: Reaction Trigger strategy for responding to hitting a cliff
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyCliff_H__
#define __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyCliff_H__

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"

namespace Anki {
namespace Cozmo {

class ReactionTriggerStrategyCliffDetected : public IReactionTriggerStrategy{
public:
  ReactionTriggerStrategyCliffDetected(Robot& robot, const Json::Value& config);

  virtual bool ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior) override;
  virtual bool ShouldResumeLastBehavior() const override {return true;}

protected:
  virtual void AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot) override;
  virtual void EnabledStateChanged(bool enabled) override
                {_shouldComputationallySwitch = false;}

private:
  bool _shouldComputationallySwitch = false;  
};


} // namespace Cozmo
} // namespace Anki

#endif //
