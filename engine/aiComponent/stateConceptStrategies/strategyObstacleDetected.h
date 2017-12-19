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

#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategy.h"

namespace Anki {
namespace Cozmo {

//Forward declarations
class ReactionObjectData;
class Robot;
  
class StrategyObstacleDetected : public IStateConceptStrategy
{
public:
  explicit StrategyObstacleDetected(const Json::Value& config);
  
protected:
  virtual bool AreStateConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_StrategyObstacleDetected_H__
