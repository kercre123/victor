/**
 * File: conditionOffTreadsState.cpp
 *
 * Author: Brad Neuman
 * Created: 2018-01-22
 *
 * Description: Checks if the off treads state matches the one supplied in config
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionOffTreadsState.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"

namespace Anki {
namespace Cozmo {

ConditionOffTreadsState::ConditionOffTreadsState(const Json::Value& config)
  : IBEICondition(config)
{
  const std::string& targetStateStr = JsonTools::ParseString(config, "targetState", "ConditionOffTreadsState.Config");  
  ANKI_VERIFY(OffTreadsStateFromString(targetStateStr, _targetState),
              "ConditionOffTreadsState.Config.IncorrectString",
              "%s is not a valid OffTreadsState",
              targetStateStr.c_str());
  _minTimeSinceChange_ms = config.get("minTimeSinceChange_ms", 0).asInt();
  if( _minTimeSinceChange_ms < 0 ) {
    _minTimeSinceChange_ms = 0;
  }
  _maxTimeSinceChange_ms = config.get("maxTimeSinceChange_ms", -1).asInt();
}

ConditionOffTreadsState::ConditionOffTreadsState(const OffTreadsState& targetState,
                                                 const std::string& ownerDebugLabel)
  : IBEICondition(IBEICondition::GenerateBaseConditionConfig(BEIConditionType::OffTreadsState))
  , _minTimeSinceChange_ms(0)
  , _maxTimeSinceChange_ms(-1)
{
  SetOwnerDebugLabel(ownerDebugLabel);
  _targetState = targetState;
}

bool ConditionOffTreadsState::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  bool ret = false;
  const OffTreadsState currState = bei.GetRobotInfo().GetOffTreadsState();
  if( currState == _targetState ) {
    const EngineTimeStamp_t currTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    const EngineTimeStamp_t lastChangedTime_ms = bei.GetRobotInfo().GetOffTreadsStateLastChangedTime_ms();
    ret = (currTime_ms - lastChangedTime_ms >= _minTimeSinceChange_ms)
          && ( (_maxTimeSinceChange_ms < 0) || (currTime_ms - lastChangedTime_ms <= _maxTimeSinceChange_ms) );
  }
  return ret;
}

}
}
