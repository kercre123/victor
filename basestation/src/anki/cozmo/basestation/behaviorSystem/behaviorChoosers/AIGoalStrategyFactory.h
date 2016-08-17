/**
 * File: AIGoalStrategyFactory
 *
 * Author: Raul
 * Created: 08/11/2016
 *
 * Description: Factory to create AIGoalStrategies from config.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_AIGoalStrategyFactory_H__
#define __Cozmo_Basestation_BehaviorSystem_AIGoalStrategyFactory_H__

#include "json/json-forwards.h"

#include <memory>

namespace Anki {
namespace Cozmo {

class Robot;
class IAIGoalStrategy;

namespace AIGoalStrategyFactory {

// creates and initializes the proper AI goal strategy from the given config. Returns a new instance if
// the config is valid, or nullptr if the configuration was not valid. Freeing the memory is responsibility
// of the caller. This factory does not store the created pointers.
IAIGoalStrategy* CreateAIGoalStrategy(Robot& robot, const Json::Value& config);

}; // namespace

}; // namespace
}; // namespace

#endif //
