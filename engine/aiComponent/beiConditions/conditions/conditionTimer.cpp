/**
 * File: strategyTimer.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-12-10
 *
 * Description: Simple strategy to become true a given time after a reset
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "engine/aiComponent/beiConditions/conditions/conditionTimer.h"

#include "anki/common/basestation/utils/timer.h"
#include "coretech/common/include/anki/common/basestation/jsonTools.h"

namespace Anki {
namespace Cozmo {

ConditionTimer::ConditionTimer(const float timeout_s)
  : IBEICondition(IBEICondition::GenerateBaseConditionConfig(BEIConditionType::Timer))
  , _timeout_s(timeout_s)
{
}

ConditionTimer::ConditionTimer(const Json::Value& config)
  : ConditionTimer( JsonTools::ParseFloat(config, "timeout", "ConditionTimer.Config.NoTimeout") )
{
}

void ConditionTimer::ResetInternal(BehaviorExternalInterface& bei)
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _timeToEnd_s = currTime_s + _timeout_s;
}

bool ConditionTimer::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  return currTime_s >= _timeToEnd_s;
}

}
}
