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
#include "engine/aiComponent/behaviorComponent/behaviors/iBehavior.h"
#include "engine/aiComponent/behaviorComponent/wantsToRunStrategies/iWantsToRunStrategy.h"
#include "anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {

namespace{
static const char* kTriggerStrategyName = "Trigger Strategy placed on charger";
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ReactionTriggerStrategyPlacedOnCharger::ReactionTriggerStrategyPlacedOnCharger(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config)
: IReactionTriggerStrategy(behaviorExternalInterface, config, kTriggerStrategyName)
{
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyPlacedOnCharger::SetupForceTriggerBehavior(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr behavior)
{
  behavior->WantsToBeActivated(behaviorExternalInterface);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ReactionTriggerStrategyPlacedOnCharger::ShouldTriggerBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr behavior)
{
  if(ANKI_VERIFY(_wantsToRunStrategy != nullptr,
                 "ReactionTriggerStrategyPlacedOnCharger.ShouldTriggerBehaviorInternal",
                 "WantsToRunStrategyNotSpecified")){
    return _wantsToRunStrategy->WantsToRun(behaviorExternalInterface);
  }
  return false;
}

  

  
} // namespace Cozmo
} // namespace Anki
