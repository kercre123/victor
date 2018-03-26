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
#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/visionScheduleMediator/visionScheduleMediator.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "webServerProcess/src/webService.h"

namespace Anki {
namespace Cozmo {

namespace {
using namespace ExternalInterface;
const char* kConditionTypeKey = "conditionType";
 const char* kWebVizModuleName = "behaviorconds";
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
, _debugLabel( MakeUniqueDebugLabel() )
, _previouslyMet( false )
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
  
  if( ANKI_DEV_CHEATS && !_isActive ) {
    // set this _conditionType instance to inactive so it gets removed from debug output
    SendInactiveToWebViz( bei );
  }
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
  const bool met = AreConditionsMetInternal(behaviorExternalInterface);
  
  if( ANKI_DEV_CHEATS ) {
    SendConditionsToWebViz( met, behaviorExternalInterface );
  }
  
  return met;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBEICondition::SendConditionsToWebViz( bool conditionsMet, BehaviorExternalInterface& bei ) const
{
  auto factors = GetDebugFactors();
  std::sort(factors.begin(), factors.end(), [](const DebugFactors& a, const DebugFactors& b) {
    return a.name < b.name;
  });
  if( _firstRun
     || (factors != _previousDebugFactorsList)
     || (conditionsMet != _previouslyMet) )
  {
    _firstRun = false;
    _previousDebugFactorsList = factors;
    _previouslyMet = conditionsMet;
    
    Json::Value json;
    json["changed"] = "factors";
    json["time"] = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    json["owner"] = _ownerLabel;
    json["name"] = GetDebugLabel();
    json["value"] = _previouslyMet;
    auto& list = json["list"];
    for( const auto& factor : GetDebugFactors() ) {
      Json::Value elem;
      elem["name"] = factor.name;
      elem["value"] = factor.value;
      list.append( elem );
    }
    const auto* webService = bei.GetRobotInfo().GetContext()->GetWebService();
    webService->SendToWebViz(kWebVizModuleName, json);
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBEICondition::SendInactiveToWebViz( BehaviorExternalInterface& bei ) const
{
  Json::Value json;
  json["changed"] = "inactive";
  json["time"] = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  json["name"] = GetDebugLabel();
  json["owner"] = _ownerLabel;
  const auto* webService = bei.GetRobotInfo().GetContext()->GetWebService();
  webService->SendToWebViz(kWebVizModuleName, json);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string IBEICondition::MakeUniqueDebugLabel() const
{
  std::string ret = BEIConditionTypeToString( _conditionType );
  static std::unordered_map<BEIConditionType, unsigned int> counts;
  const auto index = counts[_conditionType]++; // and post-increment
  if( index > 0 ) {
    ret += std::to_string(index);
  }
  return ret;
}
  
} // namespace Cozmo
} // namespace Anki
