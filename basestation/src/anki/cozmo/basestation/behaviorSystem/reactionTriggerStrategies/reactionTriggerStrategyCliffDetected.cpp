/**
 * File: reactionTriggerStrategyCliff.cpp
 *
 * Author: Kevin M. Karol
 * Created: 12/08/16
 *
 * Description: Reaction Trigger strategy for responding to
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyCliffDetected.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/robot.h"


namespace Anki {
namespace Cozmo {

namespace {
using namespace ExternalInterface;
static const char* kTriggerStrategyName = "Strategy React To Cliff";
}
  
ReactionTriggerStrategyCliffDetected::ReactionTriggerStrategyCliffDetected(Robot& robot, const Json::Value& config)
:IReactionTriggerStrategy(robot, config, kTriggerStrategyName)
{
  SubscribeToTags({{
    MessageEngineToGameTag::CliffEvent,
    MessageEngineToGameTag::RobotStopped
  }});
}
  
 
  
void ReactionTriggerStrategyCliffDetected::AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot)
{
  // we only want to switch if we haven't already triggered a behavior
  if(robot.GetBehaviorManager().GetCurrentReactionTrigger() !=
                ReactionTrigger::CliffDetected){
    switch( event.GetData().GetTag()) {
      case EngineToGameTag::CliffEvent: {
        _shouldComputationallySwitch = true;
        break;
      }
        
      case EngineToGameTag::RobotStopped: {
        _shouldComputationallySwitch = true;
        break;
      }
        
      default:
        PRINT_NAMED_ERROR("BehaviorReactToCliff.ShouldRunForEvent.BadEventType",
                          "Calling ShouldRunForEvent with an event we don't care about, this is a bug");
        break;
        
    }
  }
}
  
  
  
bool ReactionTriggerStrategyCliffDetected::ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior) {
  if(_shouldComputationallySwitch){
    _shouldComputationallySwitch = false;
    return behavior->IsRunnable(ReactionTriggerConst::kNoPreReqs);
  }
  return false;
}
  
  
} // namespace Cozmo
} // namespace Anki
