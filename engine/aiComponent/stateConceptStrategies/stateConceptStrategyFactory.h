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

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StateConceptStrategyFactory_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StateConceptStrategyFactory_H__

#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategy_fwd.h"
#include "json/json-forwards.h"

namespace Anki {
namespace Cozmo {
  
class StateConceptStrategyFactory{
public:
  static IStateConceptStrategyPtr CreateStateConceptStrategy(const Json::Value& config);
  
}; // class StateConceptStrategyFactory
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_StateConceptStrategyFactory_H__
