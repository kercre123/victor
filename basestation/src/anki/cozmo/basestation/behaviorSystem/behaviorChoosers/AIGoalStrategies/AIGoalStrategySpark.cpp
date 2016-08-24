/**
 * File: AIGoalStrategySpark
 *
 * Author: Raul
 * Created: 08/10/2016
 *
 * Description: Specific strategy for Spark goals.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "AIGoalStrategySpark.h"

#include "anki/cozmo/basestation/robot.h"
#include "anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIGoalStrategySpark::AIGoalStrategySpark(Robot& robot, const Json::Value& config)
: Anki::Cozmo::IAIGoalStrategy(config)
{
  SetGoalShouldEndSecs(FLT_MAX);
}


} // namespace
} // namespace
