/**
 * File: strategyObstacleDetected.cpp
 *
 * Author: Michael Willett
 * Created: 7/6/2017
 *
 * Description: wants to run strategy for responding to obstacle detected by prox sensor
 *
 * Copyright: Anki, Inc. 2017
 *
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/wantsToRunStrategies/strategyObstacleDetected.h"

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/aiComponent/aiComponent.h"

namespace Anki {
namespace Cozmo {
  
//////
/// React To Sudden Prox Sensor Change
/////

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StrategyObstacleDetected::StrategyObstacleDetected(Robot& robot, const Json::Value& config) 
: StrategyGeneric(robot, config)
{
  SetShouldTriggerCallback(
    [](const Robot &robot) -> bool { return robot.GetAIComponent().IsSuddenObstacleDetected(); }
  );
}

  
} // namespace Cozmo
} // namespace Anki
