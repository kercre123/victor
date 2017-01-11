/**
 * File: reactionTriggerStrategyDoubleTap.h
 *
 * Author: Kevin M. Karol
 * Created: 12/08/16
 *
 * Description: Reaction Trigger strategy for responding to double tapping a cube
 *
 * Copyright: Anki, Inc. 2016
 *
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyDoubleTap_H__
#define __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyDoubleTap_H__

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"

namespace Anki {
namespace Cozmo {

class ReactionTriggerStrategyDoubleTapDetected : public IReactionTriggerStrategy{
public:
  ReactionTriggerStrategyDoubleTapDetected(Robot& robot, const Json::Value& config);

  
  virtual bool ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior) override;
  virtual bool ShouldResumeLastBehavior() const override { return true;}
  virtual bool CanTriggerWhileTriggeredBehaviorRunning() const override { return false; }
};


} // namespace Cozmo
} // namespace Anki

#endif //
