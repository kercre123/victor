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

#include "engine/aiComponent/stateConceptStrategies/strategyObstacleDetected.h"

#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {
  
//////
/// React To Sudden Prox Sensor Change
/////

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StrategyObstacleDetected::StrategyObstacleDetected(const Json::Value& config)
: IStateConceptStrategy(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StrategyObstacleDetected::AreStateConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return behaviorExternalInterface.GetAIComponent().IsSuddenObstacleDetected();
}

  
} // namespace Cozmo
} // namespace Anki
