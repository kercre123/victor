/**
 * File: strategyObstacleDetected.h
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
 
#ifndef __Cozmo_Basestation_BehaviorSystem_StrategyObstacleDetected_H__
#define __Cozmo_Basestation_BehaviorSystem_StrategyObstacleDetected_H__

#include "engine/aiComponent/behaviorComponent/wantsToRunStrategies/strategyGeneric.h"

namespace Anki {
namespace Cozmo {

//Forward declarations
class ReactionObjectData;
class Robot;
  
class StrategyObstacleDetected : public StrategyGeneric 
{
public:
  StrategyObstacleDetected(BehaviorExternalInterface& behaviorExternalInterface,
                           IExternalInterface* robotExternalInterface,
                           const Json::Value& config);
};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_StrategyObstacleDetected_H__
