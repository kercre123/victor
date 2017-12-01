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

#include "anki/common/basestation/jsonTools.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

namespace {
using namespace ExternalInterface;
const char* kStrategyTypeKey = "strategyType";
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Json::Value IStateConceptStrategy::GenerateBaseStrategyConfig(StateConceptStrategyType type)
{
  Json::Value config;
  config[kStrategyTypeKey] = StateConceptStrategyTypeToString(type);
  return config;
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
IStateConceptStrategy::IStateConceptStrategy(const Json::Value& config)
: _strategyType(ExtractStrategyType(config))
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IStateConceptStrategy::AreStateConditionsMet(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return AreStateConditionsMetInternal(behaviorExternalInterface);
}
  
} // namespace Cozmo
} // namespace Anki
