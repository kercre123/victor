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


#include "engine/aiComponent/stateConceptStrategies/strategyTimer.h"

#include "anki/common/basestation/utils/timer.h"
#include "coretech/common/include/anki/common/basestation/jsonTools.h"

namespace Anki {
namespace Cozmo {

StrategyTimer::StrategyTimer(const float timeout_s)
  : IStateConceptStrategy(IStateConceptStrategy::GenerateBaseStrategyConfig(StateConceptStrategyType::Timer))
  , _timeout_s(timeout_s)
{
}

StrategyTimer::StrategyTimer(const Json::Value& config)
  : StrategyTimer( JsonTools::ParseFloat(config, "timeout", "StrategyTimer.Config.NoTimeout") )
{
}

void StrategyTimer::ResetInternal(BehaviorExternalInterface& bei)
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _timeToEnd_s = currTime_s + _timeout_s;
}

bool StrategyTimer::AreStateConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  return currTime_s >= _timeToEnd_s;
}

}
}
