/**
* File: beiConditionFactory.cpp
*
* Author: Kevin M. Karol
* Created: 6/03/17
*
* Description: Factory for creating beiConditions
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/beiConditions/beiConditionFactory.h"

#include "engine/aiComponent/beiConditions/conditions/conditionAnyStimuli.h"
#include "engine/aiComponent/beiConditions/conditions/conditionBatteryLevel.h"
#include "engine/aiComponent/beiConditions/conditions/conditionBeatDetected.h"
#include "engine/aiComponent/beiConditions/conditions/conditionBecameTrueThisTick.h"
#include "engine/aiComponent/beiConditions/conditions/conditionBehaviorTimer.h"
#include "engine/aiComponent/beiConditions/conditions/conditionBeingHeld.h"
#include "engine/aiComponent/beiConditions/conditions/conditionCarryingCube.h"
#include "engine/aiComponent/beiConditions/conditions/conditionCliffDetected.h"
#include "engine/aiComponent/beiConditions/conditions/conditionCompound.h"
#include "engine/aiComponent/beiConditions/conditions/conditionConnectedToCube.h"
#include "engine/aiComponent/beiConditions/conditions/conditionConsoleVar.h"
#include "engine/aiComponent/beiConditions/conditions/conditionCubeTapped.h"
#include "engine/aiComponent/beiConditions/conditions/conditionEmotion.h"
#include "engine/aiComponent/beiConditions/conditions/conditionEngineErrorCodeReceived.h"
#include "engine/aiComponent/beiConditions/conditions/conditionEyeContact.h"
#include "engine/aiComponent/beiConditions/conditions/conditionFaceKnown.h"
#include "engine/aiComponent/beiConditions/conditions/conditionFacePositionUpdated.h"
#include "engine/aiComponent/beiConditions/conditions/conditionFeatureGate.h"
#include "engine/aiComponent/beiConditions/conditions/conditionHighTemperature.h"
#include "engine/aiComponent/beiConditions/conditions/conditionIlluminationDetected.h"
#include "engine/aiComponent/beiConditions/conditions/conditionIsMaintenanceReboot.h"
#include "engine/aiComponent/beiConditions/conditions/conditionIsNightTime.h"
#include "engine/aiComponent/beiConditions/conditions/conditionMotionDetected.h"
#include "engine/aiComponent/beiConditions/conditions/conditionObjectInitialDetection.h"
#include "engine/aiComponent/beiConditions/conditions/conditionObjectKnown.h"
#include "engine/aiComponent/beiConditions/conditions/conditionObjectMoved.h"
#include "engine/aiComponent/beiConditions/conditions/conditionObjectPositionUpdated.h"
#include "engine/aiComponent/beiConditions/conditions/conditionObstacleDetected.h"
#include "engine/aiComponent/beiConditions/conditions/conditionOffTreadsState.h"
#include "engine/aiComponent/beiConditions/conditions/conditionOnCharger.h"
#include "engine/aiComponent/beiConditions/conditions/conditionOnChargerPlatform.h"
#include "engine/aiComponent/beiConditions/conditions/conditionPetInitialDetection.h"
#include "engine/aiComponent/beiConditions/conditions/conditionProxInRange.h"
#include "engine/aiComponent/beiConditions/conditions/conditionRobotInHabitat.h"
#include "engine/aiComponent/beiConditions/conditions/conditionRobotPickedUp.h"
#include "engine/aiComponent/beiConditions/conditions/conditionRobotPlacedOnSlope.h"
#include "engine/aiComponent/beiConditions/conditions/conditionRobotPoked.h"
#include "engine/aiComponent/beiConditions/conditions/conditionRobotShaken.h"
#include "engine/aiComponent/beiConditions/conditions/conditionRobotTouched.h"
#include "engine/aiComponent/beiConditions/conditions/conditionSalientPointDetected.h"
#include "engine/aiComponent/beiConditions/conditions/conditionSettingsUpdatePending.h"
#include "engine/aiComponent/beiConditions/conditions/conditionSimpleMood.h"
#include "engine/aiComponent/beiConditions/conditions/conditionStuckOnEdge.h"
#include "engine/aiComponent/beiConditions/conditions/conditionTimePowerButtonPressed.h"
#include "engine/aiComponent/beiConditions/conditions/conditionTimedDedup.h"
#include "engine/aiComponent/beiConditions/conditions/conditionTimerInRange.h"
#include "engine/aiComponent/beiConditions/conditions/conditionTriggerWordPending.h"
#include "engine/aiComponent/beiConditions/conditions/conditionTrue.h"
#include "engine/aiComponent/beiConditions/conditions/conditionUnexpectedMovement.h"
#include "engine/aiComponent/beiConditions/conditions/conditionUnitTest.h"
#include "engine/aiComponent/beiConditions/conditions/conditionUserHoldingCube.h"
#include "engine/aiComponent/beiConditions/conditions/conditionUserIntentActive.h"
#include "engine/aiComponent/beiConditions/conditions/conditionUserIntentPending.h"

#include "clad/types/behaviorComponent/beiConditionTypes.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"


namespace Anki {
namespace Vector {

namespace {
  const char* kCustomConditionKey = "customCondition";

  #define CONSOLE_GROUP "Behaviors.ConditionFactory"
  CONSOLE_VAR(bool, kDebugConditionFactory, CONSOLE_GROUP, false);

}

std::map< std::string, IBEIConditionPtr > BEIConditionFactory::_customConditionMap;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CustomBEIConditionHandleInternal::CustomBEIConditionHandleInternal(const std::string& conditionName)
  : _conditionName(conditionName)
{
  DEV_ASSERT(!_conditionName.empty(), "CustomBEIConditionHandle.NoConditionName");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CustomBEIConditionHandleInternal::~CustomBEIConditionHandleInternal()
{
  if( !_conditionName.empty() ) {
    BEIConditionFactory::RemoveCustomCondition(_conditionName);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CustomBEIConditionHandle BEIConditionFactory::InjectCustomBEICondition(const std::string& name,
                                                                       IBEIConditionPtr condition)
{
  DEV_ASSERT_MSG(_customConditionMap.find(name) == _customConditionMap.end(),
                 "BEIConditionFactory.InjectCustomBEICondition.DuplicateName",
                 "already have a condition with name '%s'",
                 name.c_str());

  _customConditionMap[name] = condition;

  if (kDebugConditionFactory) {
    PRINT_CH_DEBUG("Behaviors", "BEIConditionFactory.InjectCustomBEICondition",
                   "Added custom condition '%s'",
                   name.c_str());
  }
  {
    // set debug label to include name for easier debugging
    std::string newLabel = "@" + name;
    if( !condition->GetDebugLabel().empty() ) {
      newLabel += ":" + condition->GetDebugLabel();
    }
    condition->SetDebugLabel( newLabel );
  }

  // note: can't use make_shared because constructor is private
  CustomBEIConditionHandle ret( new CustomBEIConditionHandleInternal( name ) );
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BEIConditionFactory::RemoveCustomCondition(const std::string& name)
{
  auto it = _customConditionMap.find(name);
  if( ANKI_VERIFY( it != _customConditionMap.end(),
                   "BEIConditionFactory.RemoveCustomCondition.NotFound",
                   "condition name '%s' not found among our %zu custom conditions",
                   name.c_str(),
                   _customConditionMap.size() ) ) {
    _customConditionMap.erase(it);

    if (kDebugConditionFactory) {
      PRINT_CH_DEBUG("Behaviors", "BEIConditionFactory.RemoveCustomCondition",
                     "Removed custom condition '%s'",
                     name.c_str());
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BEIConditionFactory::IsValidCondition(const Json::Value& config)
{
  if( !config[kCustomConditionKey].isNull() ) {
    auto it = _customConditionMap.find(config[kCustomConditionKey].asString());
    const bool found = it != _customConditionMap.end();
    return found;
  }

  if( !config[IBEICondition::kConditionTypeKey].isNull() ) {
    const std::string& typeStr = config[IBEICondition::kConditionTypeKey].asString();
    BEIConditionType waste;
    const bool convertedOK =  BEIConditionTypeFromString(typeStr, waste);
    return convertedOK;
  }

  // neither key is specified
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBEIConditionPtr BEIConditionFactory::GetCustomCondition(const Json::Value& config, const std::string& ownerDebugLabel)
{
  DEV_ASSERT( config[IBEICondition::kConditionTypeKey].isNull(), "BEIConditionFactory.SpecifiedCustomConditionAndType" );

  auto it = _customConditionMap.find(config[kCustomConditionKey].asString());
  if( ANKI_VERIFY( it != _customConditionMap.end(),
                   "BEIConditionFactory.GetCustomCondition.NotFound",
                   "No custom condition with name '%s' found. Have %zu custom conditions",
                   config[kCustomConditionKey].asString().c_str(),
                   _customConditionMap.size() ) ) {
    // replace the owner debug label, even if it exists, since it was likely created before knowing
    // what behavior or condition would end up grabbing it. Obviously multiple behaviors or
    // conditions could grab it, but we can deal with that if the use of the owner debug label depends on it
    it->second->SetOwnerDebugLabel( ownerDebugLabel );
    return it->second;
  }
  else {
    return IBEIConditionPtr{};
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBEIConditionPtr BEIConditionFactory::CreateBEICondition(const Json::Value& config, const std::string& ownerDebugLabel)
{

  if( !config[kCustomConditionKey].isNull() ) {
    return GetCustomCondition(config, ownerDebugLabel);
  }

  BEIConditionType conditionType = IBEICondition::ExtractConditionType(config);

  IBEIConditionPtr condition = nullptr;

  switch (conditionType) {
    case BEIConditionType::AnyStimuli:
    {
      condition = std::make_shared<ConditionAnyStimuli>(config);
      break;
    }
    case BEIConditionType::BatteryLevel:
    {
      condition = std::make_shared<ConditionBatteryLevel>(config);
      break;
    }
    case BEIConditionType::BeatDetected:
    {
      condition = std::make_shared<ConditionBeatDetected>(config);
      break;
    }
    case BEIConditionType::BecameTrueThisTick:
    {
      condition = std::make_shared<ConditionBecameTrueThisTick>(config);
      break;
    }
    case BEIConditionType::BehaviorTimer:
    {
      condition = std::make_shared<ConditionBehaviorTimer>(config);
      break;
    }
    case BEIConditionType::CarryingCube:
    {
      condition = std::make_shared<ConditionCarryingCube>(config);
      break;
    }
    case BEIConditionType::Compound:
    {
      condition = std::make_shared<ConditionCompound>(config);
      break;
    }
    case BEIConditionType::ConsoleVar:
    {
      condition = std::make_shared<ConditionConsoleVar>(config);
      break;
    }
    case BEIConditionType::ConnectedToCube:
    {
      condition = std::make_shared<ConditionConnectedToCube>(config);
      break;
    }
    case BEIConditionType::Emotion:
    {
      condition = std::make_shared<ConditionEmotion>(config);
      break;
    }
    case BEIConditionType::EngineErrorCodeReceived:
    {
      condition = std::make_shared<ConditionEngineErrorCodeReceived>(config);
      break;
    }
    case BEIConditionType::EyeContact:
    {
      condition = std::make_shared<ConditionEyeContact>(config);
      break;
    }
    case BEIConditionType::FaceKnown:
    {
      condition = std::make_shared<ConditionFaceKnown>(config);
      break;
    }
    case BEIConditionType::FacePositionUpdated:
    {
      condition = std::make_shared<ConditionFacePositionUpdated>(config);
      break;
    }
    case BEIConditionType::FeatureGate:
    {
      condition = std::make_shared<ConditionFeatureGate>(config);
      break;
    }
    case BEIConditionType::HighTemperature:
    {
      condition = std::make_shared<ConditionHighTemperature>(config);
      break;
    }
    case BEIConditionType::IlluminationDetected:
    {
      condition = std::make_shared<ConditionIlluminationDetected>(config);
      break;
    }
    case BEIConditionType::MotionDetected:
    {
      condition = std::make_shared<ConditionMotionDetected>(config);
      break;
    }
    case BEIConditionType::ObjectInitialDetection:
    {
      condition = std::make_shared<ConditionObjectInitialDetection>(config);
      break;
    }
    case BEIConditionType::ObjectKnown:
    {
      condition = std::make_shared<ConditionObjectKnown>(config);
      break;
    }
    case BEIConditionType::ObjectMoved:
    {
      condition = std::make_shared<ConditionObjectMoved>(config);
      break;
    }
    case BEIConditionType::ObjectPositionUpdated:
    {
      condition = std::make_shared<ConditionObjectPositionUpdated>(config);
      break;
    }
    case BEIConditionType::ObstacleDetected:
    {
      condition = std::make_shared<ConditionObstacleDetected>(config);
      break;
    }
    case BEIConditionType::PetInitialDetection:
    {
      condition = std::make_shared<ConditionPetInitialDetection>(config);
      break;
    }
    case BEIConditionType::ProxInRange:
    {
      condition = std::make_shared<ConditionProxInRange>(config);
      break;
    }
    case BEIConditionType::RobotInHabitat:
    {
      condition = std::make_shared<ConditionRobotInHabitat>(config);
      break;
    }
    case BEIConditionType::RobotPickedUp:
    {
      condition = std::make_shared<ConditionRobotPickedUp>(config);
      break;
    }
    case BEIConditionType::RobotPlacedOnSlope:
    {
      condition = std::make_shared<ConditionRobotPlacedOnSlope>(config);
      break;
    }
    case BEIConditionType::RobotPoked:
    {
      condition = std::make_shared<ConditionRobotPoked>(config);
      break;
    }
    case BEIConditionType::RobotShaken:
    {
      condition = std::make_shared<ConditionRobotShaken>(config);
      break;
    }
    case BEIConditionType::RobotTouched:
    {
      condition = std::make_shared<ConditionRobotTouched>(config);
      break;
    }
    case BEIConditionType::SalientPointDetected:
    {
      condition = std::make_shared<ConditionSalientPointDetected>(config);
      break;
    }
    case BEIConditionType::SettingsUpdatePending:
    {
      condition = std::make_shared<ConditionSettingsUpdatePending>(config);
      break;
    }
    case BEIConditionType::SimpleMood:
    {
      condition = std::make_shared<ConditionSimpleMood>(config);
      break;
    }
    case BEIConditionType::TimerInRange:
    {
      condition = std::make_shared<ConditionTimerInRange>(config);
      break;
    }
    case BEIConditionType::TimedDedup:
    {
      condition = std::make_shared<ConditionTimedDedup>(config);
      break;
    }
    case BEIConditionType::TimePowerButtonPressed:
    {
      condition = std::make_shared<ConditionTimePowerButtonPressed>(config);
      break;
    }
    case BEIConditionType::TrueCondition:
    {
      condition = std::make_shared<ConditionTrue>(config);
      break;
    }
    case BEIConditionType::TriggerWordPending:
    {
      condition = std::make_shared<ConditionTriggerWordPending>(config);
      break;
    }
    case BEIConditionType::UnexpectedMovement:
    {
      condition = std::make_shared<ConditionUnexpectedMovement>(config);
      break;
    }
    case BEIConditionType::UserIntentActive:
    {
      condition = std::make_shared<ConditionUserIntentActive>(config);
      break;
    }
    case BEIConditionType::UserIntentPending:
    {
      condition = std::make_shared<ConditionUserIntentPending>(config);
      break;
    }
    case BEIConditionType::UserIsHoldingCube:
    {
      condition = std::make_shared<ConditionUserHoldingCube>(config);
      break;
    }
    case BEIConditionType::OnCharger:
    {
      condition = std::make_shared<ConditionOnCharger>(config);
      break;
    }
    case BEIConditionType::OnChargerPlatform:
    {
      condition = std::make_shared<ConditionOnChargerPlatform>(config);
      break;
    }
    case BEIConditionType::OffTreadsState:
    {
      condition = std::make_shared<ConditionOffTreadsState>(config);
      break;
    }
    case BEIConditionType::BeingHeld:
    {
      condition = std::make_shared<ConditionBeingHeld>(config);
      break;
    }
    case BEIConditionType::CliffDetected:
    {
      condition = std::make_shared<ConditionCliffDetected>(config);
      break;
    }
    case BEIConditionType::CubeTapped:
    {
      condition = std::make_shared<ConditionCubeTapped>(config);
      break;
    }
    case BEIConditionType::StuckOnEdge:
    {
      condition = std::make_shared<ConditionStuckOnEdge>(config);
      break;
    }
    case BEIConditionType::UnitTestCondition:
    {
      condition = std::make_shared<ConditionUnitTest>(config);
      break;
    }
    case BEIConditionType::IsMaintenanceReboot:
    {
      condition = std::make_shared<ConditionIsMaintenanceReboot>(config);
      break;
    }
    case BEIConditionType::IsNightTime:
    {
      condition = std::make_shared<ConditionIsNightTime>(config);
      break;
    }

    case BEIConditionType::Lambda:
    {
      DEV_ASSERT(false, "BEIConditionFactory.CreateBeiCondition.CantCreateLambdaFromConfig");
      break;
    }
    case BEIConditionType::Invalid:
    {
      DEV_ASSERT(false, "BEIConditionFactory.CreateBeiCondition.InvalidType");
      break;
    }

  }

  if( (condition != nullptr) && !ownerDebugLabel.empty() ) {
    condition->SetOwnerDebugLabel( ownerDebugLabel );
  }

  return condition;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBEIConditionPtr BEIConditionFactory::CreateBEICondition(BEIConditionType type, const std::string& ownerDebugLabel)
{
  Json::Value config = IBEICondition::GenerateBaseConditionConfig( type );
  return CreateBEICondition( config, ownerDebugLabel );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BEIConditionFactory::CheckConditionsAreUsed(const CustomBEIConditionHandleList& handles,
                                                 const std::string& debugStr)
{
  bool ret = true;

  for( const auto& handle : handles ) {
    if( handle == nullptr ) {
      ret = false;
      PRINT_NAMED_WARNING("BEIConditionFactory.AreConditionsUsed.NullHandle",
                          "One of the handles in the container was empty");
      continue;
    }

    auto it = _customConditionMap.find( handle->_conditionName );
    if( it == _customConditionMap.end() ) {
      ret = false;
      PRINT_NAMED_ERROR("BEIConditionFactory.AreConditionsUsed.HandleNotContained",
                        "The handle with name '%s' was not found in the map. This is a bug",
                        handle->_conditionName.c_str());
      continue;
    }

    const long numUses = it->second.use_count();

    if( numUses <= 1 ) {
      PRINT_NAMED_WARNING("BEIConditionFactory.AreConditionsUsed.NotUsed",
                          "%s: BEI condition '%s' only has a use count of %lu, may not have been used",
                          debugStr.c_str(),
                          handle != nullptr ? handle->_conditionName.c_str() : "<NULL>",
                          numUses);
      ret = false;
      // continue looping to print all relevant warnings
    }
  }

  return ret;
}


} // namespace Vector
} // namespace Anki
