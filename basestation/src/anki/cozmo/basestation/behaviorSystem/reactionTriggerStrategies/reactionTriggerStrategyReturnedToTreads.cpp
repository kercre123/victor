/**
 * File: reactionTriggerStrategyReturnedToTreads.cpp
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

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyReturnedToTreads.h"

#include "anki/cozmo/basestation/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {
  
namespace{
static const char* kTriggerStrategyName = "Trigger Strategy Returned to Treads";
}

  
ReactionTriggerStrategyReturnedToTreads::ReactionTriggerStrategyReturnedToTreads(Robot& robot, const Json::Value& config)
: IReactionTriggerStrategy(robot, config, kTriggerStrategyName)
{
  SubscribeToTags({
    EngineToGameTag::RobotOffTreadsStateChanged
  });
  
}

bool ReactionTriggerStrategyReturnedToTreads::ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior)
{
  if(_shouldComputationallySwitch){
    _shouldComputationallySwitch = false;
    return behavior->IsRunnable(ReactionTriggerConst::kNoPreReqs);
  }
  
  return false;
}
  
void ReactionTriggerStrategyReturnedToTreads::AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot)
{
  if( event.GetData().GetTag() != EngineToGameTag::RobotOffTreadsStateChanged ) {
    PRINT_NAMED_ERROR("BehaviorReactToReturnedToTreads.ShouldRunForEvent.InvalidTag",
                      "Received trigger event with unhandled tag %hhu",
                      event.GetData().GetTag());
    return;
  }
  
  _shouldComputationallySwitch = (event.GetData().Get_RobotOffTreadsStateChanged().treadsState == OffTreadsState::OnTreads);
}

} // namespace Cozmo
} // namespace Anki
