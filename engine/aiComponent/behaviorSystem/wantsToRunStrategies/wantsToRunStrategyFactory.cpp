/**
* File: wantsToRunStrategyFactory.cpp
*
* Author: Kevin M. Karol
* Created: 6/03/17
*
* Description: Factory for creating wantsToRunStrategy
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/behaviorSystem/wantsToRunStrategies/wantsToRunStrategyFactory.h"

#include "engine/aiComponent/behaviorSystem/wantsToRunStrategies/strategyAlwaysRun.h"
#include "engine/aiComponent/behaviorSystem/wantsToRunStrategies/strategyExpressNeedsTransition.h"
#include "engine/aiComponent/behaviorSystem/wantsToRunStrategies/strategyGeneric.h"
#include "engine/aiComponent/behaviorSystem/wantsToRunStrategies/strategyInNeedsBracket.h"
#include "engine/aiComponent/behaviorSystem/wantsToRunStrategies/strategyPlacedOnCharger.h"
#include "engine/aiComponent/behaviorSystem/wantsToRunStrategies/strategyRobotPlacedOnSlope.h"
#include "engine/aiComponent/behaviorSystem/wantsToRunStrategies/strategyRobotShaken.h"
#include "engine/aiComponent/behaviorSystem/wantsToRunStrategies/strategyObstacleDetected.h"


#include "engine/robot.h"
#include "clad/types/behaviorSystem/strategyTypes.h"

#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {

  
namespace{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IWantsToRunStrategy* WantsToRunStrategyFactory::CreateWantsToRunStrategy(BehaviorExternalInterface& behaviorExternalInterface,
                                                                         const Json::Value& config)
{

  WantsToRunStrategyType strategyType = IWantsToRunStrategy::ExtractStrategyType(config);
  
  IWantsToRunStrategy* strategy = nullptr;

  switch (strategyType) {
    case WantsToRunStrategyType::AlwaysRun:
    {
      strategy = new StrategyAlwaysRun(behaviorExternalInterface, config);
      break;
    }
    case WantsToRunStrategyType::ExpressNeedsTransition:
    {
      strategy = new StrategyExpressNeedsTransition(behaviorExternalInterface, config);
      break;
    }
    case WantsToRunStrategyType::Generic:
    {
      strategy = new StrategyGeneric(behaviorExternalInterface, config);
      break;
    }
    case WantsToRunStrategyType::InNeedsBracket:
    {
      strategy = new StrategyInNeedsBracket(behaviorExternalInterface, config);
      break;
    }
    case WantsToRunStrategyType::ObstacleDetected:
    {
      strategy = new StrategyObstacleDetected(behaviorExternalInterface, config);
      break;
    }
    case WantsToRunStrategyType::PlacedOnCharger:
    {
      strategy = new StrategyPlacedOnCharger(behaviorExternalInterface, config);
      break;
    }
    case WantsToRunStrategyType::RobotPlacedOnSlope:
    {
      strategy = new StrategyRobotPlacedOnSlope(behaviorExternalInterface, config);
      break;
    }
    case WantsToRunStrategyType::RobotShaken:
    {
      strategy = new StrategyRobotShaken(behaviorExternalInterface, config);
      break;
    }
    case WantsToRunStrategyType::Invalid:
    {
      DEV_ASSERT(false, "WantsToRunStrategyFactory.CreateWantsToRunStrategy.InvalidType");
      break;
    }
  }
  
  return strategy;
}
  
} // namespace Cozmo
} // namespace Anki
