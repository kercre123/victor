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

#include "engine/aiComponent/beiConditions/conditions/conditionCloudIntentPending.h"
#include "engine/aiComponent/beiConditions/conditions/conditionExpressNeedsTransition.h"
#include "engine/aiComponent/beiConditions/conditions/conditionFacePositionUpdated.h"
#include "engine/aiComponent/beiConditions/conditions/conditionFrustration.h"
#include "engine/aiComponent/beiConditions/conditions/conditionInNeedsBracket.h"
#include "engine/aiComponent/beiConditions/conditions/conditionMotionDetected.h"
#include "engine/aiComponent/beiConditions/conditions/conditionNegate.h"
#include "engine/aiComponent/beiConditions/conditions/conditionObjectInitialDetection.h"
#include "engine/aiComponent/beiConditions/conditions/conditionObjectMoved.h"
#include "engine/aiComponent/beiConditions/conditions/conditionObjectPositionUpdated.h"
#include "engine/aiComponent/beiConditions/conditions/conditionObstacleDetected.h"
#include "engine/aiComponent/beiConditions/conditions/conditionOffTreadsState.h"
#include "engine/aiComponent/beiConditions/conditions/conditionOnCharger.h"
#include "engine/aiComponent/beiConditions/conditions/conditionPetInitialDetection.h"
#include "engine/aiComponent/beiConditions/conditions/conditionRobotPlacedOnSlope.h"
#include "engine/aiComponent/beiConditions/conditions/conditionRobotShaken.h"
#include "engine/aiComponent/beiConditions/conditions/conditionRobotTouchGesture.h"
#include "engine/aiComponent/beiConditions/conditions/conditionTimerInRange.h"
#include "engine/aiComponent/beiConditions/conditions/conditionTrue.h"

#include "clad/types/behaviorComponent/beiConditionTypes.h"

#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {

  
namespace{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBEIConditionPtr BEIConditionFactory::CreateBEICondition(const Json::Value& config)
{

  BEIConditionType strategyType = IBEICondition::ExtractConditionType(config);
  
  IBEIConditionPtr strategy = nullptr;

  switch (strategyType) {
    case BEIConditionType::TrueCondition:
    {
      strategy = std::make_shared<ConditionTrue>(config);
      break;
    }
    case BEIConditionType::CloudIntentPending:
    {
      strategy = std::make_shared<ConditionCloudIntentPending>(config);
      break;
    }
    case BEIConditionType::ExpressNeedsTransition:
    {
      strategy = std::make_shared<ConditionExpressNeedsTransition>(config);
      break;
    }
    case BEIConditionType::FacePositionUpdated:
    {
      strategy = std::make_shared<ConditionFacePositionUpdated>(config);
      break;
    }
    case BEIConditionType::Frustration:
    {
      strategy = std::make_shared<ConditionFrustration>(config);
      break;
    }
    case BEIConditionType::InNeedsBracket:
    {
      strategy = std::make_shared<ConditionInNeedsBracket>(config);
      break;
    }
    case BEIConditionType::MotionDetected:
    {
      strategy = std::make_shared<ConditionMotionDetected>(config);
      break;
    }
    case BEIConditionType::ObjectInitialDetection:
    {
      strategy = std::make_shared<ConditionObjectInitialDetection>(config);
      break;
    }
    case BEIConditionType::ObjectMoved:
    {
      strategy = std::make_shared<ConditionObjectMoved>(config);
      break;
    }
    case BEIConditionType::ObjectPositionUpdated:
    {
      strategy = std::make_shared<ConditionObjectPositionUpdated>(config);
      break;
    }
    case BEIConditionType::ObstacleDetected:
    {
      strategy = std::make_shared<ConditionObstacleDetected>(config);
      break;
    }
    case BEIConditionType::PetInitialDetection:
    {
      strategy = std::make_shared<ConditionPetInitialDetection>(config);
      break;
    }
    case BEIConditionType::RobotPlacedOnSlope:
    {
      strategy = std::make_shared<ConditionRobotPlacedOnSlope>(config);
      break;
    }
    case BEIConditionType::RobotShaken:
    {
      strategy = std::make_shared<ConditionRobotShaken>(config);
      break;
    }
    case BEIConditionType::RobotTouchGesture:
    {
      strategy = std::make_shared<ConditionRobotTouchGesture>(config);
      break;
    }
    case BEIConditionType::TimerInRange:
    {
      strategy = std::make_shared<ConditionTimerInRange>(config);
      break;
    }
    case BEIConditionType::Negate:
    {
      strategy = std::make_shared<ConditionNegate>(config);
      break;
    }
    case BEIConditionType::OnCharger:
    {
      strategy = std::make_shared<ConditionOnCharger>(config);
      break;
    }
    case BEIConditionType::OffTreadsState:
    {
      strategy = std::make_shared<ConditionOffTreadsState>(config);
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
  
  return strategy;
}
  
} // namespace Cozmo
} // namespace Anki
