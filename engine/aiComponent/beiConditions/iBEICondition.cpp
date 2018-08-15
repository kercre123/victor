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
#include "engine/aiComponent/beiConditions/beiConditionDebugFactors.h"
#include "engine/components/visionScheduleMediator/visionScheduleMediator.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "webServerProcess/src/webService.h"

namespace Anki {
namespace Vector {

namespace {
using namespace ExternalInterface;
const char* kWebVizModuleName = "behaviorconds";
}
  
const char* const IBEICondition::kConditionTypeKey = "conditionType";
bool IBEICondition::_checkDebugFactors = true;
  
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
, _lastMetValue( false )
{
  if( ANKI_DEV_CHEATS ) {
    _debugFactors.reset( new BEIConditionDebugFactors() );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBEICondition::~IBEICondition()
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
    // TODO:(bn) this will currently get called multiple times for custom conditions if they are used as
    // operands in multiple compound conditions. I don't think there's a great way around this at the
    // moment...
    if( _conditionType != BEIConditionType::Lambda ) {
      PRINT_NAMED_WARNING("IBEICondition.Init.AlreadyInitialized", "Init called multiple times");
    }

    return;
  }
  
  InitInternal(bei);
  
  DEV_ASSERT( !GetOwnerDebugLabel().empty(), "IBEICondition.Init.MissingOwnerLabel" );
  
  _isInitialized = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBEICondition::AreConditionsMet(BehaviorExternalInterface& behaviorExternalInterface) const
{
  DEV_ASSERT(_isActive, "IBEICondition.AreConditionsMet.IsInactive");
  DEV_ASSERT(_isInitialized, "IBEICondition.AreConditionsMet.NotInitialized");
  
  // If a condition has a child condition, then setting this static flag means it won't check,
  // send, or reset debug factors. that way, the parent can have full control over when debug
  // info is sent. this is a bit hacky, but it's effect is in ANKI_DEV_CHEATS only. setting the
  // flag to false doesn't really need to be wrapped in a macro, but it makes everything a
  // no-op when shipped
  const bool checkDebugFactors = _checkDebugFactors;
  if( ANKI_DEV_CHEATS ) {
    _checkDebugFactors = false;
  }
  
  const bool met = AreConditionsMetInternal(behaviorExternalInterface);
  
  if( ANKI_DEV_CHEATS ) {
    if( checkDebugFactors ) {

      // only build factors if webviz is connected
      const auto* webService = behaviorExternalInterface.GetRobotInfo().GetContext()->GetWebService();
      if( webService != nullptr && webService->IsWebVizClientSubscribed(kWebVizModuleName) ) {
        const bool valueChanged = (_lastMetValue != met);
        _lastMetValue = met;
        auto& factors = *_debugFactors.get();
        BuildDebugFactors( factors );
        // check valueChanged too in case the sole debug factor is _lastMetValue == false.
        if( valueChanged || factors.HaveChanged() ) {
          SendConditionsToWebViz( behaviorExternalInterface );
        }
        factors.Reset();
        _checkDebugFactors = true; // other root-level conditions will be evaluated again
      }
    }
  }
  _lastMetValue = met;
  
  return met;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBEICondition::BuildDebugFactors( BEIConditionDebugFactors& factors ) const
{
  const static std::string value = "areConditionsMet";
  const static std::string name = "conditionLabel";
  const static std::string owner = "ownerDebugLabel";
  factors.AddFactor(value, _lastMetValue);
  factors.AddFactor(owner, _ownerLabel);
  factors.AddFactor(name, GetDebugLabel());
  BuildDebugFactorsInternal( factors );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const BEIConditionDebugFactors& IBEICondition::GetDebugFactors() const
{
  DEV_ASSERT(ANKI_DEV_CHEATS, "IBEICondition.GetDebugFactors.DevOnly");
  return *_debugFactors.get();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BEIConditionDebugFactors& IBEICondition::GetDebugFactors()
{
  return const_cast<BEIConditionDebugFactors&>(static_cast<const IBEICondition*>(this)->GetDebugFactors());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBEICondition::SendConditionsToWebViz( BehaviorExternalInterface& bei ) const
{
  const auto* webService = bei.GetRobotInfo().GetContext()->GetWebService();
  if( nullptr == webService || !webService->IsWebVizClientSubscribed(kWebVizModuleName) ) {
    return;
  }

  Json::Value json;
  json["time"] = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  json["factors"] = _debugFactors->GetJSON();
  webService->SendToWebViz(kWebVizModuleName, json);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBEICondition::SendInactiveToWebViz( BehaviorExternalInterface& bei ) const
{
  const auto* webService = bei.GetRobotInfo().GetContext()->GetWebService();
  if( !webService->IsWebVizClientSubscribed(kWebVizModuleName) ) {
    return;
  }
  
  Json::Value json;
  json["time"] = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  json["inactive"] = GetDebugLabel();
  json["owner"] = GetOwnerDebugLabel();
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
  
} // namespace Vector
} // namespace Anki
