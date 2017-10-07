/**
 * File: ReactionTriggerStrategyRobotPlacedOnSlope.cpp
 *
 * Author: Matt Michini
 * Created: 01/25/17
 *
 * Description: Reaction Trigger strategy for responding to
 *
 * Copyright: Anki, Inc. 2017
 *
 *
 **/


#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerStrategyRobotPlacedOnSlope.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/wantsToRunStrategies/iWantsToRunStrategy.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"

#include "anki/common/basestation/utils/timer.h"
#include "clad/types/robotStatusAndActions.h"

namespace Anki {
namespace Cozmo {
  
  
namespace{
static const char* kTriggerStrategyName = "Trigger Strategy Placed On Slope";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ReactionTriggerStrategyRobotPlacedOnSlope::ReactionTriggerStrategyRobotPlacedOnSlope(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config)
: IReactionTriggerStrategy(behaviorExternalInterface, config, kTriggerStrategyName)
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyRobotPlacedOnSlope::SetupForceTriggerBehavior(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr behavior)
{
  behavior->WantsToBeActivated(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ReactionTriggerStrategyRobotPlacedOnSlope::ShouldTriggerBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr behavior)
{
  
  if(ANKI_VERIFY(_wantsToRunStrategy != nullptr,
                 "ReactionTriggerStrategyPlacedOnCharger.ShouldTriggerBehaviorInternal",
                 "WantsToRunStrategyNotSpecified")){
    return _wantsToRunStrategy->WantsToRun(behaviorExternalInterface) && behavior->WantsToBeActivated(behaviorExternalInterface);
  }
  return false;
}

  
} // namespace Cozmo
} // namespace Anki
