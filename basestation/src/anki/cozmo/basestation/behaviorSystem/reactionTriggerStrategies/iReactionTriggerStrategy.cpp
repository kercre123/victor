/**
 * File: iReactionTriggerStrategy.cpp
 *
 * Author: Kevin M. Karol
 * Created: 12/08/16
 *
 * Description: Interface for defining when a reaction trigger should fire its
 * mapped behavior.  Strategy to behavior map is defined in behaviorManager
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerHelpers.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
#include "anki/cozmo/basestation/robot.h"



namespace Anki {
namespace Cozmo {

namespace {
using namespace ExternalInterface;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IReactionTriggerStrategy::IReactionTriggerStrategy(Robot& robot, const Json::Value& config, const std::string& strategyName)
: _robot(robot)
, _strategyName(strategyName)
, _userForcingTrigger(false)
{
  SubscribeToTags({GameToEngineTag::ExecuteReactionTrigger});
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IReactionTriggerStrategy::SubscribeToTags(std::set<GameToEngineTag> &&tags)
{
  if(_robot.HasExternalInterface()) {
    auto handlerCallback = [this](const GameToEngineEvent& event) {
      HandleEvent(event);
    };
    
    for(auto tag : tags) {
      _eventHandles.push_back(_robot.GetExternalInterface()->Subscribe(tag, handlerCallback));
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IReactionTriggerStrategy::SubscribeToTags(std::set<EngineToGameTag> &&tags)
{
  if(_robot.HasExternalInterface()) {
    auto handlerCallback = [this](const EngineToGameEvent& event) {
      HandleEvent(event);
    };
    
    for(auto tag : tags) {
      _eventHandles.push_back(_robot.GetExternalInterface()->Subscribe(tag, handlerCallback));
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IReactionTriggerStrategy::AlwaysHandle(const EngineToGameEvent& event, const Robot& robot)
{
  AlwaysHandleInternal(event, robot);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IReactionTriggerStrategy::AlwaysHandle(const GameToEngineEvent& event, const Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    case GameToEngineTag::ExecuteReactionTrigger:
    {
      Anki::Cozmo::ExternalInterface::ExecuteReactionTrigger queuedTrigger = event.GetData().Get_ExecuteReactionTrigger();
      if ( (_triggerID == queuedTrigger.reactionTriggerToBehaviorEntry.trigger) &&
          (_DebugBehaviorID == queuedTrigger.reactionTriggerToBehaviorEntry.behaviorID) )
      {
        _userForcingTrigger = true;
      }
      else if (queuedTrigger.reactionTriggerToBehaviorEntry.trigger == ReactionTrigger::NoneTrigger)
      {
        _userForcingTrigger = false;
      }
      break;
    }
    default:
    {
      AlwaysHandleInternal(event, robot);
    }
    break;
  }

}

void IReactionTriggerStrategy::BehaviorThatStrategyWillTrigger(IBehavior *behavior)
{
  _DebugBehaviorID = behavior->GetID();
  BehaviorThatStrategyWillTriggerInternal(behavior);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IReactionTriggerStrategy::ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior)
{
  //  if the user is forcing
  //  try to trigger normally but if that fails
  //  force the behavior
  if (_userForcingTrigger)
  {
    if (!ShouldTriggerBehaviorInternal(robot, behavior))
    {
      SetupForceTriggerBehavior(robot, behavior);
    }
    _userForcingTrigger = false;
    return true;
  }
  
  return ShouldTriggerBehaviorInternal(robot, behavior);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IReactionTriggerStrategy::NeedActionCompleted(const NeedsActionId needActionId)
{
  _robot.GetNeedsManager().RegisterNeedsActionCompleted(needActionId);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Util::RandomGenerator& IReactionTriggerStrategy::GetRNG() const
{
  return _robot.GetRNG();
}
  
} // namespace Cozmo
} // namespace Anki
