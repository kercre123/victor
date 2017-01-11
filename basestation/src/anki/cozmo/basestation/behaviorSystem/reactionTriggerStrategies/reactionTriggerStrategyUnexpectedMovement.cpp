/**
 * File: reactionTriggerStrategyUnexpectedMovement.cpp
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

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyUnexpectedMovement.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {

namespace{
static const char* kTriggerStrategyName = "Trigger Strategy Unexpected Movement";
}

  
ReactionTriggerStrategyUnexpectedMovement::ReactionTriggerStrategyUnexpectedMovement(Robot& robot, const Json::Value& config)
: IReactionTriggerStrategy(robot, config, kTriggerStrategyName)
{
  SubscribeToTags({
    EngineToGameTag::UnexpectedMovement
  });
}


bool ReactionTriggerStrategyUnexpectedMovement::ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior)
{
  if(_shouldComputationallySwitch){
    _shouldComputationallySwitch = false;
    return behavior->IsRunnable(ReactionTriggerConst::kNoPreReqs);
  }
  
  return false;
}


void ReactionTriggerStrategyUnexpectedMovement::AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::UnexpectedMovement:
    {
      _shouldComputationallySwitch =  true;
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("BehaviorReactToUnexpectedMovement.ShouldRunForEvent.BadEventType",
                        "Calling ShouldRunForEvent with an event we don't care about, this is a bug");
      break;
    }
  }
}

  
} // namespace Cozmo
} // namespace Anki
