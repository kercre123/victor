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

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyPlacedOnCharger.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {

namespace{
static const char* kTriggerStrategyName = "Trigger Strategy placed on charger";
const float kDisableReactionForInitialTime_sec = 20.f;
}
  
ReactionTriggerStrategyPlacedOnCharger::ReactionTriggerStrategyPlacedOnCharger(Robot& robot, const Json::Value& config)
: IReactionTriggerStrategy(robot, config, kTriggerStrategyName)
, _dontRunUntilTime_sec(-1.f)
{
  
  SubscribeToTags({
    EngineToGameTag::ChargerEvent,
  });
}
  
void ReactionTriggerStrategyPlacedOnCharger::SetupForceTriggerBehavior(const Robot& robot, const IBehavior* behavior)
{
  behavior->IsRunnable(ReactionTriggerConst::kNoPreReqs);
}
  
bool ReactionTriggerStrategyPlacedOnCharger::ShouldTriggerBehaviorInternal(const Robot& robot, const IBehavior* behavior)
{
  // This is a hack - if cozmo doesn't start on the charging points but is on the platform
  // he frequently reacts to the cliff, and we don't want to pop a modal asking the user
  // if he should go to sleep.  So don't run this reaction for the first bit until
  // we're pretty sure cozmo should be off the platform.
  const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if(_dontRunUntilTime_sec < 0){
    _dontRunUntilTime_sec =  currentTime_sec + kDisableReactionForInitialTime_sec;
  }

  if(_dontRunUntilTime_sec <= currentTime_sec &&
     _shouldTrigger){
    _shouldTrigger = false;
    return behavior->IsRunnable(ReactionTriggerConst::kNoPreReqs);
  }
  
  _shouldTrigger = false;
  return false;
}


void ReactionTriggerStrategyPlacedOnCharger::AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot)
{
  if(event.GetData().GetTag() == ExternalInterface::MessageEngineToGameTag::ChargerEvent &&
     event.GetData().Get_ChargerEvent().onCharger)
  {
    _shouldTrigger = true;
  }
}
  

  
} // namespace Cozmo
} // namespace Anki
