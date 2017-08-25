/**
 * File: ActivityStrategySpark.cpp
 *
 * Author: Raul
 * Created: 08/10/2016
 *
 * Description: Specific strategy for Spark activity.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/behaviorSystem/activities/activityStrategies/activityStrategySpark.h"

#include "engine/robot.h"
#include "anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityStrategySpark::ActivityStrategySpark(Robot& robot, const Json::Value& config)
: IActivityStrategy(robot, config)
{
  SetActivityShouldEndSecs(FLT_MAX);
}


} // namespace
} // namespace
