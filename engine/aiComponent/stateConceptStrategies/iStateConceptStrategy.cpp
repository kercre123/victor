/**
* File: iStateConceptStrategy.cpp
*
* Author: Kevin M. Karol
* Created: 7/3/17
*
* Description: Interface for generic strategies which can be used across
* all parts of the behavior system to determine when a
* behavior/reaction/activity wants to run
*
* Copyright: Anki, Inc. 2017
*
**/


#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategy.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"

#include "anki/common/basestation/jsonTools.h"

namespace Anki {
namespace Cozmo {

namespace {
using namespace ExternalInterface;
const char* kStrategyTypeKey = "strategyType";
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StateConceptStrategyType IStateConceptStrategy::ExtractStrategyType(const Json::Value& config)
{
  std::string strategyType = JsonTools::ParseString(config,
                                                    kStrategyTypeKey,
                                                    "IStateConceptStrategy.ExtractStrategyType.NoTypeSpecified");
  return StateConceptStrategyTypeFromString(strategyType);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IStateConceptStrategy::IStateConceptStrategy(BehaviorExternalInterface& behaviorExternalInterface,
                                         IExternalInterface* robotExternalInterface,
                                         const Json::Value& config)
: _behaviorExternalInterface(behaviorExternalInterface)
, _robotExternalInterface(robotExternalInterface)
, _strategyType(ExtractStrategyType(config))
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IStateConceptStrategy::SubscribeToTags(std::set<GameToEngineTag> &&tags)
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
void IStateConceptStrategy::SubscribeToTags(std::set<EngineToGameTag> &&tags)
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
void IStateConceptStrategy::AlwaysHandle(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
{
  AlwaysHandleInternal(event, behaviorExternalInterface);
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IStateConceptStrategy::AlwaysHandle(const GameToEngineEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
{
  AlwaysHandleInternal(event, behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IStateConceptStrategy::AreStateConditionsMet(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return AreStateConditionsMetInternal(behaviorExternalInterface);
}


  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Util::RandomGenerator& IStateConceptStrategy::GetRNG() const
{
  return _behaviorExternalInterface.GetRNG();
}
  
} // namespace Cozmo
} // namespace Anki
