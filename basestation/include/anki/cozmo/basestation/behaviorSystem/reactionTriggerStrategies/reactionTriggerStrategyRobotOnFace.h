/**
 * File: reactionTriggerStrategyRobotOnFace.h
 *
 * Author: Kevin M. Karol
 * Created: 12/08/16
 *
 * Description: Reaction Trigger strategy for responding to being placed on the robot's face
 *
 * Copyright: Anki, Inc. 2016
 *
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyRobotOnFace_H__
#define __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyRobotOnFace_H__

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"

namespace Anki {
namespace Cozmo {

class ReactionTriggerStrategyRobotOnFace : public IReactionTriggerStrategy{
public:
  ReactionTriggerStrategyRobotOnFace(Robot& robot, const Json::Value& config);
  
  virtual bool ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior) override;
  virtual bool ShouldResumeLastBehavior() const override { return false; }
  virtual bool CanTriggerWhileTriggeredBehaviorRunning() const override { return true; }
};


} // namespace Cozmo
} // namespace Anki

#endif //
