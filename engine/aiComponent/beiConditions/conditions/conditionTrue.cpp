/**
* File: strategyTrue.cpp
*
* Author: Raul - Kevin M. Karol
* Created: 08/10/2016 - 7/5/17
*
* Description: Strategy which always wants to run
*
* Copyright: Anki, Inc. 2016 - 2017
*
**/


#include "engine/aiComponent/beiConditions/conditions/conditionTrue.h"

#include "engine/robot.h"
#include "coretech/common/engine/utils/timer.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionTrue::ConditionTrue(const Json::Value& config, BEIConditionFactory& factory)
  : IBEICondition(config, factory)
{
}


} // namespace
} // namespace
