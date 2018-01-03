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

#include "engine/aiComponent/beiConditions/conditions/conditionAlwaysRun.h"
#include "engine/aiComponent/beiConditions/conditions/conditionCloudIntentPending.h"
#include "engine/aiComponent/beiConditions/conditions/conditionExpressNeedsTransition.h"
#include "engine/aiComponent/beiConditions/conditions/conditionFacePositionUpdated.h"
#include "engine/aiComponent/beiConditions/conditions/conditionFrustration.h"
#include "engine/aiComponent/beiConditions/conditions/conditionInNeedsBracket.h"
#include "engine/aiComponent/beiConditions/conditions/conditionObjectMoved.h"
#include "engine/aiComponent/beiConditions/conditions/conditionObjectPositionUpdated.h"
#include "engine/aiComponent/beiConditions/conditions/conditionObstacleDetected.h"
#include "engine/aiComponent/beiConditions/conditions/conditionPetInitialDetection.h"
#include "engine/aiComponent/beiConditions/conditions/conditionRobotPlacedOnSlope.h"
#include "engine/aiComponent/beiConditions/conditions/conditionRobotShaken.h"
#include "engine/aiComponent/beiConditions/conditions/conditionRobotTouchGesture.h"
#include "engine/aiComponent/beiConditions/conditions/conditionTimer.h"


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
    case BEIConditionType::AlwaysRun:
    {
      strategy = std::make_shared<ConditionAlwaysRun>(config);
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
    case BEIConditionType::Timer:
    {
      strategy = std::make_shared<ConditionTimer>(config);
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
