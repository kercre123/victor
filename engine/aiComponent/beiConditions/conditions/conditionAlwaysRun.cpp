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


#include "engine/aiComponent/beiConditions/conditions/conditionAlwaysRun.h"

#include "engine/robot.h"
#include "anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionAlwaysRun::ConditionAlwaysRun(const Json::Value& config)
: IBEICondition(config)
{
}


} // namespace
} // namespace
