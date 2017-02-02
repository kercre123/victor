/**
 * File: reactionTriggerStrategyPyramid.h
 *
 * Author: Kevin M. Karol
 * Created: 12/08/16
 *
 * Description: Reaction Trigger strategy for responding to seeing a pyramid
 *
 * Copyright: Anki, Inc. 2016
 *
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyPyramid_H__
#define __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyPyramid_H__


#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"

namespace Anki {
namespace Cozmo {

class ReactionTriggerStrategyPyramidInitialDetection : public IReactionTriggerStrategy{
public:
  ReactionTriggerStrategyPyramidInitialDetection(Robot& robot, const Json::Value& config);

  // Currently disabled until we want to respond to pyramids for some reason
  virtual bool ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior) override { return false; }
  virtual bool ShouldResumeLastBehavior() const override { return true; }
  virtual bool CanTriggerWhileTriggeredBehaviorRunning() const override { return true; }
};


} // namespace Cozmo
} // namespace Anki

#endif //
