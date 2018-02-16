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

#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/components/visionScheduleMediator/visionScheduleMediator.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

namespace {
using namespace ExternalInterface;
const char* kConditionTypeKey = "conditionType";
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Json::Value IBEICondition::GenerateBaseConditionConfig(BEIConditionType type)
{
  Json::Value config;
  config[kConditionTypeKey] = BEIConditionTypeToString(type);
  return config;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BEIConditionType IBEICondition::ExtractConditionType(const Json::Value& config)
{
  std::string strategyType = JsonTools::ParseString(config,
                                                    kConditionTypeKey,
                                                    "IBEICondition.ExtractConditionType.NoTypeSpecified");
  return BEIConditionTypeFromString(strategyType);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBEICondition::IBEICondition(const Json::Value& config)
: _conditionType(ExtractConditionType(config))
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBEICondition::SetActive(BehaviorExternalInterface& bei, bool setToActive)
{
  // Activate required VisionModes(if any)
  std::set<VisionModeRequest> visionModeRequests;
  GetRequiredVisionModes(visionModeRequests);
  if(!visionModeRequests.empty()){
    if(setToActive){
      bei.GetVisionScheduleMediator().SetVisionModeSubscriptions(this, visionModeRequests);
    } else {
      bei.GetVisionScheduleMediator().ReleaseAllVisionModeSubscriptions(this);
    }
  }

  _isActive = setToActive;
  SetActiveInternal(bei, _isActive);
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
  DEV_ASSERT(_isActive, "IBEICondition.AreConditionsMet.IsInactive");
  DEV_ASSERT(_isInitialized, "IBEICondition.AreConditionsMet.NotInitialized");
  return AreConditionsMetInternal(behaviorExternalInterface);
}
  
} // namespace Cozmo
} // namespace Anki
