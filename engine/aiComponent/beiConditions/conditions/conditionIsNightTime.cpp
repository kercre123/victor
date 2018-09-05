/**
 * File: conditionIsNightTime.cpp
 *
 * Author: Brad Neuman
 * Created: 2018-09-04
 *
 * Description: Checks with the sleep tracker if it is currently considered to be night time
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionIsNightTime.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/sleepTracker.h"

namespace Anki {
namespace Vector {

ConditionIsNightTime::ConditionIsNightTime(const Json::Value& config)
  : IBEICondition(config)
{
}

bool ConditionIsNightTime::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  const bool isNight = bei.GetSleepTracker().IsNightTime();
  return isNight;
}
  
} // namespace
} // namespace


