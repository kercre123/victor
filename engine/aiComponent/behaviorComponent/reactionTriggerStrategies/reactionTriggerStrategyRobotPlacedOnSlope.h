/**
 * File: ReactionTriggerStrategyRobotPlacedOnSlope.h
 *
 * Author: Matt Michini
 * Created: 01/25/17
 *
 * Description: Reaction Trigger strategy for responding to robot being placed down on an incline/slope.
 *
 * Copyright: Anki, Inc. 2017
 *
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyRobotPlacedOnSlope_H__
#define __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyRobotPlacedOnSlope_H__

#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/iReactionTriggerStrategy.h"

namespace Anki {
namespace Cozmo {

class ReactionTriggerStrategyRobotPlacedOnSlope : public IReactionTriggerStrategy{
public:
  ReactionTriggerStrategyRobotPlacedOnSlope(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config);

  virtual bool ShouldResumeLastBehavior() const override { return false; }
  virtual bool CanInterruptOtherTriggeredBehavior() const override { return true; }

protected:
  virtual bool ShouldTriggerBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr behavior) override;
  virtual void SetupForceTriggerBehavior(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr behavior) override;
  
};


} // namespace Cozmo
} // namespace Anki

#endif //
