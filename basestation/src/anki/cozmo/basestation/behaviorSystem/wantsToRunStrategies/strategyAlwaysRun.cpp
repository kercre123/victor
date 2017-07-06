/**
* File: strategyAlwaysRun.cpp
*
* Author: Raul - Kevin M. Karol
* Created: 08/10/2016 - 7/5/17
*
* Description: Strategy which always wants to run
*
* Copyright: Anki, Inc. 2016 - 2017
*
**/


#include "anki/cozmo/basestation/behaviorSystem/wantsToRunStrategies/strategyAlwaysRun.h"

#include "anki/cozmo/basestation/robot.h"
#include "anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StrategyAlwaysRun::StrategyAlwaysRun(Robot& robot, const Json::Value& config)
: IWantsToRunStrategy(robot, config)
{
}


} // namespace
} // namespace
