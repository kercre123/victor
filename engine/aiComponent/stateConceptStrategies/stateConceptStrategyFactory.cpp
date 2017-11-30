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

#include "engine/aiComponent/stateConceptStrategies/stateConceptStrategyFactory.h"

#include "engine/aiComponent/stateConceptStrategies/strategyAlwaysRun.h"
#include "engine/aiComponent/stateConceptStrategies/strategyCloudIntentPending.h"
#include "engine/aiComponent/stateConceptStrategies/strategyExpressNeedsTransition.h"
#include "engine/aiComponent/stateConceptStrategies/strategyFacePositionUpdated.h"
#include "engine/aiComponent/stateConceptStrategies/strategyFrustration.h"
#include "engine/aiComponent/stateConceptStrategies/strategyGeneric.h"
#include "engine/aiComponent/stateConceptStrategies/strategyInNeedsBracket.h"
#include "engine/aiComponent/stateConceptStrategies/strategyObjectMoved.h"
#include "engine/aiComponent/stateConceptStrategies/strategyObjectPositionUpdated.h"
#include "engine/aiComponent/stateConceptStrategies/strategyObstacleDetected.h"
#include "engine/aiComponent/stateConceptStrategies/strategyPetInitialDetection.h"
#include "engine/aiComponent/stateConceptStrategies/strategyRobotPlacedOnSlope.h"
#include "engine/aiComponent/stateConceptStrategies/strategyRobotShaken.h"
#include "engine/aiComponent/stateConceptStrategies/strategyRobotTouchGesture.h"


#include "engine/robot.h"
#include "clad/types/behaviorComponent/strategyTypes.h"

#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {

  
namespace{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IStateConceptStrategyPtr StateConceptStrategyFactory::CreateStateConceptStrategy(BehaviorExternalInterface& bei,
                                                                                 IExternalInterface* rei,
                                                                                 const Json::Value& config)
{

  StateConceptStrategyType strategyType = IStateConceptStrategy::ExtractStrategyType(config);
  
  IStateConceptStrategyPtr strategy = nullptr;

  switch (strategyType) {
    case StateConceptStrategyType::AlwaysRun:
    {
      strategy = std::make_shared<StrategyAlwaysRun>(bei, rei, config);
      break;
    }
    case StateConceptStrategyType::CloudIntentPending:
    {
      strategy = std::make_shared<StrategyCloudIntentPending>(bei, rei, config);
      break;
    }
    case StateConceptStrategyType::ExpressNeedsTransition:
    {
      strategy = std::make_shared<StrategyExpressNeedsTransition>(bei, rei, config);
      break;
    }
    case StateConceptStrategyType::FacePositionUpdated:
    {
      strategy = std::make_shared<StrategyFacePositionUpdated>(bei, rei, config);
      break;
    }
    case StateConceptStrategyType::Frustration:
    {
      strategy = std::make_shared<StrategyFrustration>(bei, rei, config);
      break;
    }
    case StateConceptStrategyType::Generic:
    {
      strategy = std::make_shared<StrategyGeneric>(bei, rei, config);
      break;
    }
    case StateConceptStrategyType::InNeedsBracket:
    {
      strategy = std::make_shared<StrategyInNeedsBracket>(bei, rei, config);
      break;
    }
    case StateConceptStrategyType::ObjectMoved:
    {
      strategy = std::make_shared<StrategyObjectMoved>(bei, rei, config);
      break;
    }
    case StateConceptStrategyType::ObjectPositionUpdated:
    {
      strategy = std::make_shared<StrategyObjectPositionUpdated>(bei, rei, config);
      break;
    }
    case StateConceptStrategyType::ObstacleDetected:
    {
      strategy = std::make_shared<StrategyObstacleDetected>(bei, rei, config);
      break;
    }
    case StateConceptStrategyType::PetInitialDetection:
    {
      strategy = std::make_shared<StrategyPetInitialDetection>(bei, rei, config);
      break;
    }
    case StateConceptStrategyType::RobotPlacedOnSlope:
    {
      strategy = std::make_shared<StrategyRobotPlacedOnSlope>(bei, rei, config);
      break;
    }
    case StateConceptStrategyType::RobotShaken:
    {
      strategy = std::make_shared<StrategyRobotShaken>(bei, rei, config);
      break;
    }
    case StateConceptStrategyType::RobotTouchGesture:
    {
      strategy = std::make_shared<StrategyRobotTouchGesture>(bei, rei, config);
      break;
    }
    case StateConceptStrategyType::Invalid:
    {
      DEV_ASSERT(false, "StateConceptStrategyFactory.CreateWantsToRunStrategy.InvalidType");
      break;
    }
  }
  
  return strategy;
}
  
} // namespace Cozmo
} // namespace Anki
