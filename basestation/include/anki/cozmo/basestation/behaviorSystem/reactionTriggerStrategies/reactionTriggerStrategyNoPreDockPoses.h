/**
 * File: reactionTriggerStrategyNoPreDockPoses.h
 *
 * Author: Kevin M. Karol
 * Created: 12/08/16
 *
 * Description: Reaction Trigger strategy for responding to a cube moving
 *
 * Copyright: Anki, Inc. 2016
 *
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyNoPreDockPoses_H__
#define __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyNoPreDockPoses_H__

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"
#include "anki/common/basestation/objectIDs.h"

namespace Anki {
namespace Cozmo {

//Forward declarations
class ReactionObjectData;
class Robot;
  
class ReactionTriggerStrategyNoPreDockPoses : public IReactionTriggerStrategy{
public:
  ReactionTriggerStrategyNoPreDockPoses(Robot& robot, const Json::Value& config);

  virtual bool ShouldResumeLastBehavior() const override { return false;}
  virtual bool CanInterruptOtherTriggeredBehavior() const override { return true; }
  virtual bool CanInterruptSelf() const override { return false; }

protected:
  virtual bool ShouldTriggerBehaviorInternal(const Robot& robot, const IBehavior* behavior) override;
  virtual void SetupForceTriggerBehavior(const Robot& robot, const IBehavior* behavior) override;

private:
  
};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyNoPreDockPoses_H__
