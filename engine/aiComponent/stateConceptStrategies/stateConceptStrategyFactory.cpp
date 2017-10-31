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
#include "engine/aiComponent/stateConceptStrategies/strategyGeneric.h"
#include "engine/aiComponent/stateConceptStrategies/strategyInNeedsBracket.h"
#include "engine/aiComponent/stateConceptStrategies/strategyPlacedOnCharger.h"
#include "engine/aiComponent/stateConceptStrategies/strategyRobotPlacedOnSlope.h"
#include "engine/aiComponent/stateConceptStrategies/strategyRobotShaken.h"
#include "engine/aiComponent/stateConceptStrategies/strategyObstacleDetected.h"
#include "engine/aiComponent/stateConceptStrategies/strategyRobotTouchGesture.h"


#include "engine/robot.h"
#include "clad/types/behaviorComponent/strategyTypes.h"

#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {

  
namespace{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IStateConceptStrategy* StateConceptStrategyFactory::CreateStateConceptStrategy(BehaviorExternalInterface& behaviorExternalInterface,
                                                                               IExternalInterface* robotExternalInterface,
                                                                               const Json::Value& config)
{

  StateConceptStrategyType strategyType = IStateConceptStrategy::ExtractStrategyType(config);
  
  IStateConceptStrategy* strategy = nullptr;

  switch (strategyType) {
    case StateConceptStrategyType::AlwaysRun:
    {
      strategy = new StrategyAlwaysRun(behaviorExternalInterface, robotExternalInterface, config);
      break;
    }
    case StateConceptStrategyType::CloudIntentPending:
    {
      strategy = new StrategyCloudIntentPending(behaviorExternalInterface, robotExternalInterface, config);
      break;
    }
    case StateConceptStrategyType::ExpressNeedsTransition:
    {
      strategy = new StrategyExpressNeedsTransition(behaviorExternalInterface, robotExternalInterface, config);
      break;
    }
    case StateConceptStrategyType::Generic:
    {
      strategy = new StrategyGeneric(behaviorExternalInterface, robotExternalInterface, config);
      break;
    }
    case StateConceptStrategyType::InNeedsBracket:
    {
      strategy = new StrategyInNeedsBracket(behaviorExternalInterface, robotExternalInterface, config);
      break;
    }
    case StateConceptStrategyType::ObstacleDetected:
    {
      strategy = new StrategyObstacleDetected(behaviorExternalInterface, robotExternalInterface, config);
      break;
    }
    case StateConceptStrategyType::PlacedOnCharger:
    {
      strategy = new StrategyPlacedOnCharger(behaviorExternalInterface, robotExternalInterface, config);
      break;
    }
    case StateConceptStrategyType::RobotPlacedOnSlope:
    {
      strategy = new StrategyRobotPlacedOnSlope(behaviorExternalInterface, robotExternalInterface, config);
      break;
    }
    case StateConceptStrategyType::RobotShaken:
    {
      strategy = new StrategyRobotShaken(behaviorExternalInterface, robotExternalInterface, config);
      break;
    }
    case StateConceptStrategyType::RobotTouchGesture:
    {
      strategy = new StrategyRobotTouchGesture(behaviorExternalInterface, robotExternalInterface, config);
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
