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

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"

namespace Anki {
namespace Cozmo {

class ReactionTriggerStrategySparked : public IReactionTriggerStrategy{
public:
  ReactionTriggerStrategySparked(Robot& robot, const Json::Value& config);
  
  virtual bool ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior) override;
  virtual bool ShouldResumeLastBehavior() const override { return false;}  
  virtual bool CanInterruptOtherTriggeredBehavior() const override { return true; }
};


} // namespace Cozmo
} // namespace Anki

#endif //
