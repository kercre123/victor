/**
* File: strategyPlacedOnCharger.h
*
* Author: Kevin M. Karol
* Created: 12/08/16
*
* Description: Strategy which wants to run when Cozmo has just been placed
* on a charger
*
* Copyright: Anki, Inc. 2016
*
*
**/

#include "engine/aiComponent/stateConceptStrategies/strategyPlacedOnCharger.h"

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {

namespace{
const float kDisableReactionForInitialTime_sec = 20.f;
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StrategyPlacedOnCharger::StrategyPlacedOnCharger(BehaviorExternalInterface& behaviorExternalInterface,
                                                 IExternalInterface* robotExternalInterface,
                                                 const Json::Value& config)
: IStateConceptStrategy(behaviorExternalInterface, robotExternalInterface, config)
, _dontRunUntilTime_sec(-1.f)
{
  
  SubscribeToTags({
    EngineToGameTag::ChargerEvent,
  });
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StrategyPlacedOnCharger::AreStateConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
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
    return true;;
  }
  
  _shouldTrigger = false;
  return false;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StrategyPlacedOnCharger::AlwaysHandleInternal(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
{
  if(event.GetData().GetTag() == ExternalInterface::MessageEngineToGameTag::ChargerEvent)
  {
    _shouldTrigger = event.GetData().Get_ChargerEvent().onCharger;
  }
}
  

  
} // namespace Cozmo
} // namespace Anki
