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
#include "anki/cozmo/basestation/robot.h"



namespace Anki {
namespace Cozmo {

namespace {
using namespace ExternalInterface;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IReactionTriggerStrategy::IReactionTriggerStrategy(Robot& robot, const Json::Value& config, const std::string& strategyName)
:  _robot(robot)
,  _strategyName(strategyName)
{
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
Util::RandomGenerator& IReactionTriggerStrategy::GetRNG() const
{
  return _robot.GetRNG();
}
  
} // namespace Cozmo
} // namespace Anki
