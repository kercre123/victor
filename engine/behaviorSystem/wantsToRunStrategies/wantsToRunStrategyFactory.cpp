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

#include "engine/behaviorSystem/wantsToRunStrategies/wantsToRunStrategyFactory.h"

#include "engine/behaviorSystem/wantsToRunStrategies/strategyAlwaysRun.h"
#include "engine/behaviorSystem/wantsToRunStrategies/strategyExpressNeedsTransition.h"
#include "engine/behaviorSystem/wantsToRunStrategies/strategyGeneric.h"
#include "engine/behaviorSystem/wantsToRunStrategies/strategyInNeedsBracket.h"
#include "engine/behaviorSystem/wantsToRunStrategies/strategyPlacedOnCharger.h"
#include "engine/behaviorSystem/wantsToRunStrategies/strategyRobotPlacedOnSlope.h"
#include "engine/behaviorSystem/wantsToRunStrategies/strategyRobotShaken.h"
#include "engine/behaviorSystem/wantsToRunStrategies/strategyObstacleDetected.h"


#include "engine/robot.h"
#include "clad/types/behaviorSystem/strategyTypes.h"

#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {

  
namespace{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IWantsToRunStrategy* WantsToRunStrategyFactory::CreateWantsToRunStrategy(Robot& robot,
                                                                         const Json::Value& config)
{

  WantsToRunStrategyType strategyType = IWantsToRunStrategy::ExtractStrategyType(config);
  
  IWantsToRunStrategy* strategy = nullptr;

  switch (strategyType) {
    case WantsToRunStrategyType::AlwaysRun:
    {
      strategy = new StrategyAlwaysRun(robot, config);
      break;
    }
    case WantsToRunStrategyType::ExpressNeedsTransition:
    {
      strategy = new StrategyExpressNeedsTransition(robot, config);
      break;
    }
    case WantsToRunStrategyType::Generic:
    {
      strategy = new StrategyGeneric(robot, config);
      break;
    }
    case WantsToRunStrategyType::InNeedsBracket:
    {
      strategy = new StrategyInNeedsBracket(robot, config);
      break;
    }
    case WantsToRunStrategyType::ObstacleDetected:
    {
      strategy = new StrategyObstacleDetected(robot, config);
      break;
    }
    case WantsToRunStrategyType::PlacedOnCharger:
    {
      strategy = new StrategyPlacedOnCharger(robot, config);
      break;
    }
    case WantsToRunStrategyType::RobotPlacedOnSlope:
    {
      strategy = new StrategyRobotPlacedOnSlope(robot, config);
      break;
    }
    case WantsToRunStrategyType::RobotShaken:
    {
      strategy = new StrategyRobotShaken(robot, config);
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
