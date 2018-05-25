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
#include "engine/aiComponent/beiConditions/beiConditionDebugFactors.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

CONSOLE_VAR_EXTERN(float, kTimeMultiplier);
  
namespace {

const char* kManualResetKey = "manualResetOnly";
const char* kBeginRangeKey = "begin_s";
const char* kEndRangeKey = "end_s";

}

ConditionTimerInRange::ConditionTimerInRange(const Json::Value& config)
  : IBEICondition(config)
{
  const bool gotBegin = JsonTools::GetValueOptional(config, kBeginRangeKey, _params._rangeBegin_s);
  const bool gotEnd = JsonTools::GetValueOptional(config, kEndRangeKey, _params._rangeEnd_s);
  JsonTools::GetValueOptional(config, kManualResetKey, _params._manualResetOnly);

  ANKI_VERIFY( gotBegin || gotEnd, "ConditionTimerInRange.BadConfig", "Need to specify at least one of '%s' or '%s'",
               kBeginRangeKey,
               kEndRangeKey );
}

void ConditionTimerInRange::SetActiveInternal(BehaviorExternalInterface& bei, bool setActive)
{
  if( !_params._manualResetOnly ) {
    Reset();
  }
}
  
void ConditionTimerInRange::Reset()
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _timeReset = currTime_s;
}

bool ConditionTimerInRange::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const float timerVal = currTime_s - _timeReset;

  const float ffwdTime = kTimeMultiplier * timerVal;
  return _params._rangeBegin_s <= ffwdTime && ffwdTime < _params._rangeEnd_s;
}
  
void ConditionTimerInRange::BuildDebugFactorsInternal( BEIConditionDebugFactors& factors ) const
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const float timerVal = currTime_s - _timeReset;
  const float ffwdTime = kTimeMultiplier * timerVal;
  factors.AddFactor( "min_t", _params._rangeBegin_s );
  factors.AddFactor( "max_t", _params._rangeEnd_s );
  factors.AddFactor( "cur_t", ffwdTime );
}
  
};

}
