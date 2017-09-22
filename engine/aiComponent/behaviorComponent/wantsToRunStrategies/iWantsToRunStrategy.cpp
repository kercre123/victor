/**
* File: iWantsToRunStrategy.cpp
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


#include "engine/aiComponent/behaviorComponent/wantsToRunStrategies/iWantsToRunStrategy.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

namespace {
using namespace ExternalInterface;
const char* kStrategyTypeKey = "strategyType";
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WantsToRunStrategyType IWantsToRunStrategy::ExtractStrategyType(const Json::Value& config)
{
  std::string strategyType = JsonTools::ParseString(config,
                                                    kStrategyTypeKey,
                                                    "IWantsToRunStrategy.ExtractStrategyType.NoTypeSpecified");
  return WantsToRunStrategyTypeFromString(strategyType);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IWantsToRunStrategy::IWantsToRunStrategy(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config)
: _behaviorExternalInterface(behaviorExternalInterface)
, _strategyType(ExtractStrategyType(config))
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IWantsToRunStrategy::SubscribeToTags(std::set<GameToEngineTag> &&tags)
{
  auto robotExternalInterface = _behaviorExternalInterface.GetRobotExternalInterface().lock();
  if(robotExternalInterface != nullptr) {
    auto handlerCallback = [this](const GameToEngineEvent& event) {
      HandleEvent(event);
    };
    
    for(auto tag : tags) {
      _eventHandles.push_back(robotExternalInterface->Subscribe(tag, handlerCallback));
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IWantsToRunStrategy::SubscribeToTags(std::set<EngineToGameTag> &&tags)
{
  auto robotExternalInterface = _behaviorExternalInterface.GetRobotExternalInterface().lock();
  if(robotExternalInterface != nullptr) {
    auto handlerCallback = [this](const EngineToGameEvent& event) {
      HandleEvent(event);
    };
    
    for(auto tag : tags) {
      _eventHandles.push_back(robotExternalInterface->Subscribe(tag, handlerCallback));
    }
  }
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IWantsToRunStrategy::AlwaysHandle(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
{
  AlwaysHandleInternal(event, behaviorExternalInterface);
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IWantsToRunStrategy::AlwaysHandle(const GameToEngineEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
{
  AlwaysHandleInternal(event, behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IWantsToRunStrategy::WantsToRun(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return WantsToRunInternal(behaviorExternalInterface);
}


  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Util::RandomGenerator& IWantsToRunStrategy::GetRNG() const
{
  return _behaviorExternalInterface.GetRNG();
}
  
} // namespace Cozmo
} // namespace Anki
