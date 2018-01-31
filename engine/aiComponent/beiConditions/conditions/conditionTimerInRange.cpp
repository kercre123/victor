/**
 * File: strategyTimerInRange.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-12-10
 *
 * Description: Simple strategy to become true a given time after a reset
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "engine/aiComponent/beiConditions/conditions/conditionTimerInRange.h"

#include "coretech/common/engine/utils/timer.h"
#include "coretech/common/engine/jsonTools.h"

namespace Anki {
namespace Cozmo {

namespace {

const char* kBeginRangeKey = "begin_s";
const char* kEndRangeKey = "end_s";

}

ConditionTimerInRange::ConditionTimerInRange(const Json::Value& config)
  : IBEICondition(config)
{
  const bool gotBegin = JsonTools::GetValueOptional(config, kBeginRangeKey, _params._rangeBegin_s);
  const bool gotEnd = JsonTools::GetValueOptional(config, kEndRangeKey, _params._rangeEnd_s);

  ANKI_VERIFY( gotBegin || gotEnd, "ConditionTimerInRange.BadConfig", "Need to specify at least one of '%s' or '%s'",
               kBeginRangeKey,
               kEndRangeKey );
}

void ConditionTimerInRange::ResetInternal(BehaviorExternalInterface& bei)
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _timeReset = currTime_s;
}

bool ConditionTimerInRange::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const float timerVal = currTime_s - _timeReset;

  return _params._rangeBegin_s <= timerVal && timerVal < _params._rangeEnd_s;
}

}
}
