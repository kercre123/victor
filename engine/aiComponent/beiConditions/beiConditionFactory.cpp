/**
* File: stateConceptStrategyFactory.cpp
*
* Author: Kevin M. Karol
* Created: 6/03/17
*
* Description: Factory for creating wantsToRunStrategy
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/beiConditions/beiConditionFactory.h"

#include "engine/aiComponent/beiConditions/customBeiConditionContainer.h"

#include "engine/aiComponent/beiConditions/conditions/conditionBatteryLevel.h"
#include "engine/aiComponent/beiConditions/conditions/conditionBehaviorTimer.h"
#include "engine/aiComponent/beiConditions/conditions/conditionCliffDetected.h"
#include "engine/aiComponent/beiConditions/conditions/conditionCompound.h"
#include "engine/aiComponent/beiConditions/conditions/conditionConsoleVar.h"
#include "engine/aiComponent/beiConditions/conditions/conditionCubeTapped.h"
#include "engine/aiComponent/beiConditions/conditions/conditionEmotion.h"
#include "engine/aiComponent/beiConditions/conditions/conditionEyeContact.h"
#include "engine/aiComponent/beiConditions/conditions/conditionFacePositionUpdated.h"
#include "engine/aiComponent/beiConditions/conditions/conditionFeatureGate.h"
#include "engine/aiComponent/beiConditions/conditions/conditionMotionDetected.h"
#include "engine/aiComponent/beiConditions/conditions/conditionObjectInitialDetection.h"
#include "engine/aiComponent/beiConditions/conditions/conditionObjectMoved.h"
#include "engine/aiComponent/beiConditions/conditions/conditionObjectPositionUpdated.h"
#include "engine/aiComponent/beiConditions/conditions/conditionObstacleDetected.h"
#include "engine/aiComponent/beiConditions/conditions/conditionOffTreadsState.h"
#include "engine/aiComponent/beiConditions/conditions/conditionOnCharger.h"
#include "engine/aiComponent/beiConditions/conditions/conditionPetInitialDetection.h"
#include "engine/aiComponent/beiConditions/conditions/conditionProxInRange.h"
#include "engine/aiComponent/beiConditions/conditions/conditionRobotPlacedOnSlope.h"
#include "engine/aiComponent/beiConditions/conditions/conditionRobotShaken.h"
#include "engine/aiComponent/beiConditions/conditions/conditionRobotTouched.h"
#include "engine/aiComponent/beiConditions/conditions/conditionSimpleMood.h"
#include "engine/aiComponent/beiConditions/conditions/conditionTimedDedup.h"
#include "engine/aiComponent/beiConditions/conditions/conditionTimerInRange.h"
#include "engine/aiComponent/beiConditions/conditions/conditionTriggerWordPending.h"
#include "engine/aiComponent/beiConditions/conditions/conditionUnexpectedMovement.h"
#include "engine/aiComponent/beiConditions/conditions/conditionUnitTest.h"
#include "engine/aiComponent/beiConditions/conditions/conditionUserIntentPending.h"
#include "engine/aiComponent/beiConditions/conditions/conditionTrue.h"

#include "clad/types/behaviorComponent/beiConditionTypes.h"

#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {

  
namespace{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BEIConditionFactory::BEIConditionFactory()
  : IDependencyManagedComponent<AIComponentID>(this, AIComponentID::BEIConditionFactory)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BEIConditionFactory::~BEIConditionFactory()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBEIConditionPtr BEIConditionFactory::CreateBEICondition(const Json::Value& config, const std::string& ownerDebugLabel)
{

  BEIConditionType strategyType = IBEICondition::ExtractConditionType(config);
  
  IBEIConditionPtr strategy = nullptr;

  switch (strategyType) {
    case BEIConditionType::BatteryLevel:
    {
      strategy = std::make_shared<ConditionBatteryLevel>(config, *this);
      break;
    }
    case BEIConditionType::BehaviorTimer:
    {
      strategy = std::make_shared<ConditionBehaviorTimer>(config, *this);
      break;
    }
    case BEIConditionType::Compound:
    {
      strategy = std::make_shared<ConditionCompound>(config, *this);
      break;
    }
    case BEIConditionType::ConsoleVar:
    {
      strategy = std::make_shared<ConditionConsoleVar>(config, *this);
      break;
    }
    case BEIConditionType::Emotion:
    {
      strategy = std::make_shared<ConditionEmotion>(config, *this);
      break;
    }
    case BEIConditionType::EyeContact:
    {
      strategy = std::make_shared<ConditionEyeContact>(config, *this);
      break;
    }
    case BEIConditionType::FacePositionUpdated:
    {
      strategy = std::make_shared<ConditionFacePositionUpdated>(config, *this);
      break;
    }
    case BEIConditionType::FeatureGate:
    {
      strategy = std::make_shared<ConditionFeatureGate>(config, *this);
      break;
    }
    case BEIConditionType::MotionDetected:
    {
      strategy = std::make_shared<ConditionMotionDetected>(config, *this);
      break;
    }
    case BEIConditionType::ObjectInitialDetection:
    {
      strategy = std::make_shared<ConditionObjectInitialDetection>(config, *this);
      break;
    }
    case BEIConditionType::ObjectMoved:
    {
      strategy = std::make_shared<ConditionObjectMoved>(config, *this);
      break;
    }
    case BEIConditionType::ObjectPositionUpdated:
    {
      strategy = std::make_shared<ConditionObjectPositionUpdated>(config, *this);
      break;
    }
    case BEIConditionType::ObstacleDetected:
    {
      strategy = std::make_shared<ConditionObstacleDetected>(config, *this);
      break;
    }
    case BEIConditionType::PetInitialDetection:
    {
      strategy = std::make_shared<ConditionPetInitialDetection>(config, *this);
      break;
    }
    case BEIConditionType::ProxInRange:
    {
      strategy = std::make_shared<ConditionProxInRange>(config, *this);
      break;
    }
    case BEIConditionType::RobotPlacedOnSlope:
    {
      strategy = std::make_shared<ConditionRobotPlacedOnSlope>(config, *this);
      break;
    }
    case BEIConditionType::RobotShaken:
    {
      strategy = std::make_shared<ConditionRobotShaken>(config, *this);
      break;
    }
    case BEIConditionType::RobotTouched:
    {
      strategy = std::make_shared<ConditionRobotTouched>(config, *this);
      break;
    }
    case BEIConditionType::SimpleMood:
    {
      strategy = std::make_shared<ConditionSimpleMood>(config, *this);
      break;
    }
    case BEIConditionType::TimerInRange:
    {
      strategy = std::make_shared<ConditionTimerInRange>(config, *this);
      break;
    }
    case BEIConditionType::TimedDedup:
    {
      strategy = std::make_shared<ConditionTimedDedup>(config, *this);
      break;
    }
    case BEIConditionType::TrueCondition:
    {
      strategy = std::make_shared<ConditionTrue>(config, *this);
      break;
    }
    case BEIConditionType::TriggerWordPending:
    {
      strategy = std::make_shared<ConditionTriggerWordPending>(config, *this);
      break;
    }
    case BEIConditionType::UnexpectedMovement:
    {
      strategy = std::make_shared<ConditionUnexpectedMovement>(config, *this);
      break;
    }
    case BEIConditionType::UserIntentPending:
    {
      strategy = std::make_shared<ConditionUserIntentPending>(config, *this);
      break;
    }
    case BEIConditionType::OnCharger:
    {
      strategy = std::make_shared<ConditionOnCharger>(config, *this);
      break;
    }
    case BEIConditionType::OffTreadsState:
    {
      strategy = std::make_shared<ConditionOffTreadsState>(config, *this);
      break;
    }
    case BEIConditionType::CliffDetected:
    {
      strategy = std::make_shared<ConditionCliffDetected>(config, *this);
      break;
    }
    case BEIConditionType::CubeTapped:
    {
      strategy = std::make_shared<ConditionCubeTapped>(config, *this);
      break;
    }
    case BEIConditionType::UnitTestCondition:
    {
      strategy = std::make_shared<ConditionUnitTest>(config, *this);
      break;
    }
    
    case BEIConditionType::Lambda:
    {
      DEV_ASSERT(false, "BEIConditionFactory.CreateWantsToRunStrategy.CantCreateLambdaFromConfig");
      break;
    }
    case BEIConditionType::Invalid:
    {
      DEV_ASSERT(false, "BEIConditionFactory.CreateWantsToRunStrategy.InvalidType");
      break;
    }
    
  }
  
  if( (strategy != nullptr) && !ownerDebugLabel.empty() ) {
    strategy->SetOwnerDebugLabel( ownerDebugLabel );
  }
  
  return strategy;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBEIConditionPtr BEIConditionFactory::CreateBEICondition(BEIConditionType type, const std::string& ownerDebugLabel)
{
  Json::Value config = IBEICondition::GenerateBaseConditionConfig( type );
  return CreateBEICondition( config, ownerDebugLabel );
}
  
} // namespace Cozmo
} // namespace Anki
