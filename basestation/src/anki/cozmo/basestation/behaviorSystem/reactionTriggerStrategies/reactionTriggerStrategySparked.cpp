/**
 * File: reactionTriggerStrategySparked.cpp
 *
 * Author: Kevin M. Karol
 * Created: 12/08/16
 *
 * Description: Reaction Trigger strategy for responding to
 *
 * Copyright: Anki, Inc. 2016
 *
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategySparked.h"

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {

namespace{
static const char* kTriggerStrategyName = "Trigger strategy Sparked";
}
  
ReactionTriggerStrategySparked::ReactionTriggerStrategySparked(Robot& robot, const Json::Value& config)
: IReactionTriggerStrategy(robot, config, kTriggerStrategyName)
{
  
}

  
  
bool ReactionTriggerStrategySparked::ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior)
{
  bool currentBehaviorIsReaction = robot.GetBehaviorManager().CurrentBehaviorTriggeredAsReaction();
  if(!currentBehaviorIsReaction){
    return false;
  }
  
  const bool cancelCurrentReaction = robot.GetBehaviorManager().ShouldSwitchToSpark();
  
  const IBehavior* currentBehavior = robot.GetBehaviorManager().GetCurrentBehavior();
  const bool behaviorWhitelisted = (currentBehavior != nullptr &&
                                    ((currentBehavior->GetClass() == BehaviorClass::ReactToCliff) ||
                                     (currentBehavior->GetClass() == BehaviorClass::ReactToSparked)));
  
  return cancelCurrentReaction && !behaviorWhitelisted && behavior->IsRunnable(ReactionTriggerConst::kNoPreReqs);
}

  
} // namespace Cozmo
} // namespace Anki
