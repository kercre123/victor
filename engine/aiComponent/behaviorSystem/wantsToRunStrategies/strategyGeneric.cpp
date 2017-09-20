/**
 * File: strategyGeneric.cpp
 *
 * Author: Lee Crippen - Al Chaussee
 * Created: 06/20/2017 - 07/11/2017
 *
 * Description: Generic strategy for responding to many configurations of events.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorSystem/wantsToRunStrategies/strategyGeneric.h"

#include "anki/common/basestation/jsonTools.h"

namespace Anki {
namespace Cozmo {

StrategyGeneric::StrategyGeneric(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config)
: Anki::Cozmo::IWantsToRunStrategy(behaviorExternalInterface, config)
{
  
}

void StrategyGeneric::ConfigureRelevantEvents(std::set<EngineToGameTag> relevantEvents,
                                              E2GHandleCallbackType callback)
{
  if(!ANKI_VERIFY(_relevantEngineToGameEvents.empty(),
                  "ReactionTriggerStrategyGeneric::SetEventHandleCallback.EventsAlreadySet", ""))
  {
    return;
  }
  
  if(!ANKI_VERIFY(!relevantEvents.empty(),
                  "ReactionTriggerStrategyGeneric::SetEventHandleCallback.NewEventsNotSpecified", ""))
  {
    return;
  }
  
  _relevantEngineToGameEvents = relevantEvents; // keep a copy on this instance for validation later
  SubscribeToTags(std::move(relevantEvents));
  _engineToGameHandleCallback = callback;
}

void StrategyGeneric::ConfigureRelevantEvents(std::set<GameToEngineTag> relevantEvents,
                                              G2EHandleCallbackType callback)
{
  if(!ANKI_VERIFY(_relevantGameToEngineEvents.empty(),
                  "ReactionTriggerStrategyGeneric::SetEventHandleCallback.EventsAlreadySet", ""))
  {
    return;
  }
  
  if(!ANKI_VERIFY(!relevantEvents.empty(),
                  "ReactionTriggerStrategyGeneric::SetEventHandleCallback.NewEventsNotSpecified", ""))
  {
    return;
  }
  
  _relevantGameToEngineEvents = relevantEvents; // keep a copy on this instance for validation later
  SubscribeToTags(std::move(relevantEvents));
  _gameToEngineHandleCallback = callback;
}

bool StrategyGeneric::WantsToRunInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const bool wantsToRun = _wantsToRun;
  const bool shouldTrigger = ShouldTrigger(behaviorExternalInterface);
  
  _wantsToRun = false;
  
  return wantsToRun || shouldTrigger;
}

void StrategyGeneric::AlwaysHandleInternal(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
{
  if (_relevantEngineToGameEvents.find(event.GetData().GetTag()) == _relevantEngineToGameEvents.end())
  {
    PRINT_NAMED_ERROR("ReactionTriggerStrategyGeneric.AlwaysHandleInternal.BadEventType",
                      "GenericStrategy not configured to handle tag type %s",
                      MessageEngineToGameTagToString(event.GetData().GetTag()));
    return;
  }
  
  if (!_engineToGameHandleCallback || _engineToGameHandleCallback(event, behaviorExternalInterface))
  {
    _wantsToRun = true;
  }
}

void StrategyGeneric::AlwaysHandleInternal(const GameToEngineEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
{
  if (_relevantGameToEngineEvents.find(event.GetData().GetTag()) == _relevantGameToEngineEvents.end())
  {
    PRINT_NAMED_ERROR("ReactionTriggerStrategyGeneric.AlwaysHandleInternal.BadEventType",
                      "GenericStrategy not configured to handle tag type %s",
                      MessageGameToEngineTagToString(event.GetData().GetTag()));
    return;
  }
  
  if (!_gameToEngineHandleCallback || _gameToEngineHandleCallback(event, behaviorExternalInterface))
  {
    _wantsToRun = true;
  }
}

bool StrategyGeneric::ShouldTrigger(BehaviorExternalInterface& behaviorExternalInterface) const
{
  if(_relevantEngineToGameEvents.empty() &&
     _relevantGameToEngineEvents.empty() &&
     _shouldTriggerCallback != nullptr)
  {
    return _shouldTriggerCallback(behaviorExternalInterface);
  }
  
  return false;
}


}
}
