/**
 * File: reactionTriggerStrategyOnCharger.cpp
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

#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerStrategyPlacedOnCharger.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategy.h"
#include "anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {

namespace{
static const char* kTriggerStrategyName = "Trigger Strategy placed on charger";
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ReactionTriggerStrategyPlacedOnCharger::ReactionTriggerStrategyPlacedOnCharger(BehaviorExternalInterface& behaviorExternalInterface,
                                                                               IExternalInterface* robotExternalInterface,
                                                                               const Json::Value& config)
: IReactionTriggerStrategy(behaviorExternalInterface, robotExternalInterface,
                           config, kTriggerStrategyName)
{
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyPlacedOnCharger::SetupForceTriggerBehavior(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr behavior)
{
  behavior->WantsToBeActivated(behaviorExternalInterface);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ReactionTriggerStrategyPlacedOnCharger::ShouldTriggerBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr behavior)
{
  if(ANKI_VERIFY(_stateConceptStrategy != nullptr,
                 "ReactionTriggerStrategyPlacedOnCharger.ShouldTriggerBehaviorInternal",
                 "WantsToRunStrategyNotSpecified")){
    return _stateConceptStrategy->AreStateConditionsMet(behaviorExternalInterface);
  }
  return false;
}

  

  
} // namespace Cozmo
} // namespace Anki
