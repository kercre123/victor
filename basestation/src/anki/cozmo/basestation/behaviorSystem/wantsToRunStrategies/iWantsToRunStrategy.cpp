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


#include "anki/cozmo/basestation/behaviorSystem/wantsToRunStrategies/iWantsToRunStrategy.h"

#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"

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
IWantsToRunStrategy::IWantsToRunStrategy(Robot& robot, const Json::Value& config)
: _robot(robot)
, _strategyType(ExtractStrategyType(config))
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IWantsToRunStrategy::SubscribeToTags(std::set<GameToEngineTag> &&tags)
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
void IWantsToRunStrategy::SubscribeToTags(std::set<EngineToGameTag> &&tags)
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
void IWantsToRunStrategy::AlwaysHandle(const EngineToGameEvent& event, const Robot& robot)
{
  AlwaysHandleInternal(event, robot);
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IWantsToRunStrategy::AlwaysHandle(const GameToEngineEvent& event, const Robot& robot)
{
  AlwaysHandleInternal(event, robot);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IWantsToRunStrategy::WantsToRun(const Robot& robot) const
{
  return WantsToRunInternal(robot);
}


  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Util::RandomGenerator& IWantsToRunStrategy::GetRNG() const
{
  return _robot.GetRNG();
}
  
} // namespace Cozmo
} // namespace Anki
