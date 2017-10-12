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

#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/iReactionTriggerStrategy.h"

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerHelpers.h"
#include "engine/aiComponent/behaviorComponent/wantsToRunStrategies/wantsToRunStrategyFactory.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/robot.h"

#include "clad/types/behaviorSystem/strategyTypes.h"

namespace Anki {
namespace Cozmo {

namespace {
using namespace ExternalInterface;
static const char* kWantsToRunStrategyConfigKey = "wantsToRunStrategyConfig";
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IReactionTriggerStrategy::IReactionTriggerStrategy(BehaviorExternalInterface& behaviorExternalInterface,
                                                   IExternalInterface* robotExternalInterface,
                                                   const Json::Value& config, const std::string& strategyName)
: _wantsToRunStrategy(nullptr)
, _behaviorExternalInterface(behaviorExternalInterface)
, _robotExternalInterface(robotExternalInterface)
, _strategyName(strategyName)
, _userForcingTrigger(false)
{
  SubscribeToTags({GameToEngineTag::ExecuteReactionTrigger});
  
  
  if(config.isMember(kWantsToRunStrategyConfigKey)){
    const Json::Value& wantsToRunConfig = config[kWantsToRunStrategyConfigKey];
    _wantsToRunStrategy.reset(WantsToRunStrategyFactory::CreateWantsToRunStrategy(behaviorExternalInterface,
                                                                                  robotExternalInterface,
                                                                                  wantsToRunConfig));
  }
  
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IReactionTriggerStrategy::SubscribeToTags(std::set<GameToEngineTag> &&tags)
{
  if(_robotExternalInterface != nullptr) {
    auto handlerCallback = [this](const GameToEngineEvent& event) {
      HandleEvent(event);
    };
    
    for(auto tag : tags) {
      _eventHandles.push_back(_robotExternalInterface->Subscribe(tag, handlerCallback));
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IReactionTriggerStrategy::SubscribeToTags(std::set<EngineToGameTag> &&tags)
{
  if(_robotExternalInterface != nullptr) {
    auto handlerCallback = [this](const EngineToGameEvent& event) {
      HandleEvent(event);
    };
    
    for(auto tag : tags) {
      _eventHandles.push_back(_robotExternalInterface->Subscribe(tag, handlerCallback));
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IReactionTriggerStrategy::AlwaysHandle(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
{
  AlwaysHandleInternal(event, _behaviorExternalInterface);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IReactionTriggerStrategy::AlwaysHandle(const GameToEngineEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
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
      AlwaysHandleInternal(event, _behaviorExternalInterface);
    }
    break;
  }

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IReactionTriggerStrategy::BehaviorThatStrategyWillTrigger(ICozmoBehaviorPtr behavior)
{
  _DebugBehaviorID = behavior->GetID();
  BehaviorThatStrategyWillTriggerInternal(behavior);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IReactionTriggerStrategy::ShouldTriggerBehavior(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr behavior)
{
  //  if the user is forcing
  //  try to trigger normally but if that fails
  //  force the behavior
  if (_userForcingTrigger)
  {
    if (!ShouldTriggerBehaviorInternal(behaviorExternalInterface, behavior))
    {
      SetupForceTriggerBehavior(behaviorExternalInterface, behavior);
    }
    _userForcingTrigger = false;
    return true;
  }
  
  return ShouldTriggerBehaviorInternal(behaviorExternalInterface, behavior);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IReactionTriggerStrategy::NeedActionCompleted(NeedsActionId needsActionId)
{
  if(_behaviorExternalInterface.HasNeedsManager()){
    _behaviorExternalInterface.GetNeedsManager().RegisterNeedsActionCompleted(needsActionId);
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Util::RandomGenerator& IReactionTriggerStrategy::GetRNG() const
{
  return _behaviorExternalInterface.GetRNG();
}
  
} // namespace Cozmo
} // namespace Anki
