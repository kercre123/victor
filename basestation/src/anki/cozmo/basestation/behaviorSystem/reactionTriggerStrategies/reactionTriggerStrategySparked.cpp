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
  
  // Since there are situations where a fist bump could play as a celebratory reaction to a successful spark completion,
  // but we remain in sparksBehaviorChooser, we're doing a more specific check here that allows us to cancel the currently
  // running reaction if any new spark is requested before the previous spark has technically completed.
  // We used to call ShouldSwitchToSpark() but that would be false because _activeSpark is still not UnlockId::Count from the previous spark.
  // Also, I think this only makes sense when going into a new spark and not from spark to non-spark.
  // This also won't do anything if re-activating the same spark as before.
  const bool cancelCurrentReaction = (robot.GetBehaviorManager().GetRequestedSpark() != UnlockId::Count) &&
                                     (robot.GetBehaviorManager().GetActiveSpark() != robot.GetBehaviorManager().GetRequestedSpark());
  
  const IBehavior* currentBehavior = robot.GetBehaviorManager().GetCurrentBehavior();
  const bool behaviorWhitelisted = (currentBehavior != nullptr &&
                                    ((currentBehavior->GetClass() == BehaviorClass::ReactToCliff) ||
                                     (currentBehavior->GetClass() == BehaviorClass::ReactToSparked)));
  
  return cancelCurrentReaction && !behaviorWhitelisted && behavior->IsRunnable(ReactionTriggerConst::kNoPreReqs);
}

  
} // namespace Cozmo
} // namespace Anki
