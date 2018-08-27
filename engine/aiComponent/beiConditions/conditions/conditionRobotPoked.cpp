/**
 * File: conditionRobotPoked.cpp
 *
 * Author: Guillermo Bautista
 * Created: 2018/08/21
 *
 * Description: Condition that indicates when Victor has been poked recently.
 *
 * Copyright: Anki, Inc. 2018
 *
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionRobotPoked.h"

#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "util/logging/logging.h"

#define LOG_CHANNEL "Behaviors"

#define DEBUG_CONDITION_ROBOT_POKED false

namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionRobotPoked::ConditionRobotPoked(const Json::Value& config)
: IBEICondition(config)
{
  _wasPokedRecentlyTimeThreshold_ms = 1000 * JsonTools::ParseFloat(config, "wasPokedRecentlyTimeThreshold_sec", "ConditionRobotPoked.Config");
  LOG_DEBUG("ConditionRobotPoked.ParseConfig", "Time threshold set to %u ms", _wasPokedRecentlyTimeThreshold_ms);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionRobotPoked::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
  
  // Only considered Poked if robot has been alerted to a poke event recently:
  const auto& msSinceLastPoke = robotInfo.GetTimeSinceLastPoke_ms();
  const bool wasPokedRecently = (msSinceLastPoke <= _wasPokedRecentlyTimeThreshold_ms);
#if DEBUG_CONDITION_ROBOT_POKED
  LOG_DEBUG("ConditionRobotPoked.AreConditionsMetInternal",
            "Robot %s poked recently enough, last poke was %u ms ago",
            (wasPokedRecently ? "WAS" : "WAS NOT"), msSinceLastPoke);
#endif
  return wasPokedRecently;
}

} // namespace Vector
} // namespace Anki
