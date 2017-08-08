/**
 * File: ActivityStrategyFactory.h
 *
 * Author: Raul
 * Created: 08/11/2016
 *
 * Description: Factory to create ActivityStrategies from config.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_Activities_ActivityStrategies_ActivityStrategyFactory_H__
#define __Cozmo_Basestation_BehaviorSystem_Activities_ActivityStrategies_ActivityStrategyFactory_H__

#include "json/json-forwards.h"

namespace Anki {
namespace Cozmo {

class Robot;
class IActivityStrategy;

namespace ActivityStrategyFactory {

// creates and initializes the proper activity strategy from the given config. Returns a new instance if
// the config is valid, or nullptr if the configuration was not valid. Freeing the memory is responsibility
// of the caller. This factory does not store the created pointers.
IActivityStrategy* CreateActivityStrategy(Robot& robot, const Json::Value& config);

}; // namespace

}; // namespace
}; // namespace

#endif // __Cozmo_Basestation_BehaviorSystem_Activities_ActivityStrategies_ActivityStrategyFactory_H__
