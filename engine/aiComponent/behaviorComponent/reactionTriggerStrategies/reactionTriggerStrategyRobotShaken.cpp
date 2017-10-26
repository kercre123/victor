/**
 * File: reactionTriggerStrategyRobotShaken.cpp
 *
 * Author: Matt Michini
 * Created: 2017/01/11
 *
 * Description: Reaction Trigger strategy for responding to robot being shaken
 *
 * Copyright: Anki, Inc. 2017
 *
 *
 **/

#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerStrategyRobotShaken.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategy.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {
  
namespace{
static const char* kTriggerStrategyName = "Trigger strategy robot shaken";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ReactionTriggerStrategyRobotShaken::ReactionTriggerStrategyRobotShaken(BehaviorExternalInterface& behaviorExternalInterface,
                                                                       IExternalInterface* robotExternalInterface,
                                                                       const Json::Value& config)
: IReactionTriggerStrategy(behaviorExternalInterface, robotExternalInterface,
                           config, kTriggerStrategyName)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyRobotShaken::SetupForceTriggerBehavior(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr behavior)
{
  behavior->WantsToBeActivated(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ReactionTriggerStrategyRobotShaken::ShouldTriggerBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr behavior)
{
  if(ANKI_VERIFY(_stateConceptStrategy != nullptr,
                 "ReactionTriggerStrategyNoPreDockPoses.ShouldTriggerBehaviorInternal",
                 "WantsToRunStrategyNotSpecified")){
    const bool isActivatable = behavior->IsActivated() || behavior->WantsToBeActivated(behaviorExternalInterface);

    return _stateConceptStrategy->AreStateConditionsMet(behaviorExternalInterface) && isActivatable;
  }
  
  return false;
}

} // namespace Cozmo
} // namespace Anki
