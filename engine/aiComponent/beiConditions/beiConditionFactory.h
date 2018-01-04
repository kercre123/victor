/**
* File: stateConceptStrategyFactory.h
*
* Author: Kevin M. Karol
* Created: 6/03/17
*
* Description: Factory for creating wantsToRunStrategy
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_BEIConditionFactory_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_BEIConditionFactory_H__

#include "engine/aiComponent/beiConditions/iBEICondition_fwd.h"
#include "json/json-forwards.h"

namespace Anki {
namespace Cozmo {
  
class BEIConditionFactory{
public:
  static IBEIConditionPtr CreateBEICondition(const Json::Value& config);
  
}; // class BEIConditionFactory
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_BEIConditionFactory_H__
