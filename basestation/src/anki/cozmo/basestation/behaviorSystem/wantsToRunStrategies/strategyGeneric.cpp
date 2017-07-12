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

#include "anki/cozmo/basestation/behaviorSystem/wantsToRunStrategies/strategyGeneric.h"

#include "anki/common/basestation/jsonTools.h"

namespace Anki {
namespace Cozmo {

StrategyGeneric::StrategyGeneric(Robot& robot, const Json::Value& config)
: Anki::Cozmo::IWantsToRunStrategy(robot, config)
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

bool StrategyGeneric::WantsToRunInternal(const Robot& robot) const
{
  const bool wantsToRun = _wantsToRun;
  const bool shouldTrigger = ShouldTrigger(robot);
  
  _wantsToRun = false;
  
  return wantsToRun || shouldTrigger;
}

void StrategyGeneric::AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot)
{
  if (_relevantEngineToGameEvents.find(event.GetData().GetTag()) == _relevantEngineToGameEvents.end())
  {
    PRINT_NAMED_ERROR("ReactionTriggerStrategyGeneric.AlwaysHandleInternal.BadEventType",
                      "GenericStrategy not configured to handle tag type %s",
                      MessageEngineToGameTagToString(event.GetData().GetTag()));
    return;
  }
  
  if (!_engineToGameHandleCallback || _engineToGameHandleCallback(event, robot))
  {
    _wantsToRun = true;
  }
}

void StrategyGeneric::AlwaysHandleInternal(const GameToEngineEvent& event, const Robot& robot)
{
  if (_relevantGameToEngineEvents.find(event.GetData().GetTag()) == _relevantGameToEngineEvents.end())
  {
    PRINT_NAMED_ERROR("ReactionTriggerStrategyGeneric.AlwaysHandleInternal.BadEventType",
                      "GenericStrategy not configured to handle tag type %s",
                      MessageGameToEngineTagToString(event.GetData().GetTag()));
    return;
  }
  
  if (!_gameToEngineHandleCallback || _gameToEngineHandleCallback(event, robot))
  {
    _wantsToRun = true;
  }
}

bool StrategyGeneric::ShouldTrigger(const Robot& robot) const
{
  if(_relevantEngineToGameEvents.empty() &&
     _relevantGameToEngineEvents.empty() &&
     _shouldTriggerCallback != nullptr)
  {
    return _shouldTriggerCallback(robot);
  }
  
  return false;
}


}
}
