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


#include "engine/aiComponent/beiConditions/iBEICondition.h"

#include "anki/common/basestation/jsonTools.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

namespace {
using namespace ExternalInterface;
const char* kStrategyTypeKey = "strategyType";
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Json::Value IBEICondition::GenerateBaseConditionConfig(BEIConditionType type)
{
  Json::Value config;
  config[kStrategyTypeKey] = BEIConditionTypeToString(type);
  return config;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BEIConditionType IBEICondition::ExtractConditionType(const Json::Value& config)
{
  std::string strategyType = JsonTools::ParseString(config,
                                                    kStrategyTypeKey,
                                                    "IBEICondition.ExtractConditionType.NoTypeSpecified");
  return BEIConditionTypeFromString(strategyType);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBEICondition::IBEICondition(const Json::Value& config)
: _conditionType(ExtractConditionType(config))
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBEICondition::Reset(BehaviorExternalInterface& bei)
{
  _hasEverBeenReset = true;
  ResetInternal(bei);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBEICondition::Init(BehaviorExternalInterface& bei)
{
  if( _isInitialized ) {
    PRINT_NAMED_WARNING("IBEICondition.Init.AlreadyInitialized", "Init called multiple times");
  }
  
  InitInternal(bei);
  
  _isInitialized = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBEICondition::AreConditionsMet(BehaviorExternalInterface& behaviorExternalInterface) const
{
  DEV_ASSERT(_hasEverBeenReset, "IBEICondition.AreConditionsMet.NotEverReset");
  DEV_ASSERT(_isInitialized, "IBEICondition.AreConditionsMet.NotInitialized");
  return AreConditionsMetInternal(behaviorExternalInterface);
}
  
} // namespace Cozmo
} // namespace Anki
