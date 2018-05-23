/**
* File: conditionAnyStimuli.cpp
*
* Author: ross
* Created: May 15 2018
*
* Description: Condition that standardizes the params of stimuli conditions, so you only specify the stimuli.
*
* Copyright: Anki, Inc. 2018
*
**/


#include "engine/aiComponent/beiConditions/conditions/conditionAnyStimuli.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "engine/aiComponent/beiConditions/conditions/conditionFaceKnown.h"
#include "engine/aiComponent/beiConditions/conditions/conditionMotionDetected.h"
#include "coretech/common/engine/jsonTools.h"


namespace Anki {
namespace Cozmo {

namespace{
  const static std::set<BEIConditionType> kSupportedTypes = {
    BEIConditionType::MotionDetected,
    BEIConditionType::FaceKnown,
    BEIConditionType::FacePositionUpdated,
    BEIConditionType::RobotTouched,
    BEIConditionType::RobotShaken,
  };
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionAnyStimuli::ConditionAnyStimuli(const Json::Value& config)
  : IBEICondition(config)
{
  // config requires a bool for all types in kSupportedTypes above, like
  //  {
  //     "MotionDetected": true,
  //     "FaceKnown": false,
  //     "FacePositionUpdated": true,
  //     etc
  //  }
  for( const auto& type : kSupportedTypes ) {
    const char* typeStr = BEIConditionTypeToString(type);
    bool typeUsed = JsonTools::ParseBool(config,
                                         typeStr,
                                         "ConditionAnyStimuli.Ctor.MissingRequired");
    if( typeUsed ) {
      AddCondition( type );
    }
  }
  ANKI_VERIFY( !_conditions.empty(), "ConditionAnyStimuli.Ctor.NoStimuli", "At least one type must be true");
  ANKI_VERIFY( config.getMemberNames().size() == 1 + kSupportedTypes.size(),
               "ConditionAnyStimuli.Ctor.ExtraArgs",
               "You may have provided extra args!" );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionAnyStimuli::ConditionAnyStimuli(const std::set<BEIConditionType>& types)
  : IBEICondition( IBEICondition::GenerateBaseConditionConfig(BEIConditionType::AnyStimuli) )
{
  for( const auto& type : types ) {
    if( ANKI_VERIFY( std::find(kSupportedTypes.begin(), kSupportedTypes.end(), type) != kSupportedTypes.end(),
                     "ConditionAnyStimuli.Ctor.InvalidType",
                     "'%s' is not a supported type",
                     BEIConditionTypeToString(type) ) )
    {
      AddCondition( type );
    }
  }
  ANKI_VERIFY( !_conditions.empty(), "ConditionAnyStimuli.Ctor.NoStimuli", "At least one type must be true");
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionAnyStimuli::AddCondition( BEIConditionType type )
{
  Json::Value config = IBEICondition::GenerateBaseConditionConfig( type );
  switch( type ) {
    case BEIConditionType::MotionDetected:
    {
      config["motionArea"] = "Any";
      config["motionLevel"] = "Low";
    }
      break;
    case BEIConditionType::FaceKnown:
    {
      config["maxFaceDist_mm"] = 2000;
    }
      break;
    case BEIConditionType::RobotTouched:
    {
      config["minTouchTime"] = 0.1;
    }
    case BEIConditionType::FacePositionUpdated:
    case BEIConditionType::RobotShaken:
    {
      // no config
    }
      break;
    default:
      break;
  }
  IBEIConditionPtr cond = BEIConditionFactory::CreateBEICondition( config, GetDebugLabel() );
  _conditions.push_back( cond );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionAnyStimuli::InitInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  for( auto& cond : _conditions ) {
    cond->Init( behaviorExternalInterface );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionAnyStimuli::GetRequiredVisionModes(std::set<VisionModeRequest>& requiredVisionModes) const
{
  for( const auto& cond : _conditions ) {
    std::set<VisionModeRequest> condModes;
    cond->GetRequiredVisionModes( condModes );
    requiredVisionModes.insert( condModes.begin(), condModes.end() );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionAnyStimuli::SetActiveInternal(BehaviorExternalInterface& behaviorExternalInterface, bool active)
{
  for( auto& cond : _conditions ) {
    cond->SetActive( behaviorExternalInterface, active );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionAnyStimuli::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  for( const auto& cond : _conditions ) {
    if( cond->AreConditionsMet( behaviorExternalInterface ) ) {
      return true;
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBEICondition::DebugFactorsList ConditionAnyStimuli::GetDebugFactors() const
{
  DebugFactorsList ret;
  for( const auto& cond : _conditions ) {
    ret.emplace_back(BEIConditionTypeToString(cond->GetConditionType()), "used"); // this isnt very useful, but GetDebugFactors needs a revamp
  }
  return ret;
}

} // namespace
} // namespace
