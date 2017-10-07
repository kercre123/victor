/**
* File: wantsToRunStrategyFactory.h
*
* Author: Kevin M. Karol
* Created: 6/03/17
*
* Description: Factory for creating wantsToRunStrategy
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_WantsToRunStrategyFactory_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_WantsToRunStrategyFactory_H__

#include "anki/common/basestation/jsonTools.h"

namespace Anki {
namespace Cozmo {
// forward declarations
class BehaviorExternalInterface;
class IExternalInterface;
class IWantsToRunStrategy;
class Robot;
  
class WantsToRunStrategyFactory{
public:
  static IWantsToRunStrategy* CreateWantsToRunStrategy(BehaviorExternalInterface& behaviorExternalInterface,
                                                       IExternalInterface* robotExternalInterface,
                                                       const Json::Value& config);
  
}; // class WantsToRunStrategyFactory
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_WantsToRunStrategyFactory_H__
