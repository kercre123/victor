/**
 * File: conditionRobotRollInRange.cpp
 *
 * Author: Guillermo Bautista
 * Created: 2019/02/27
 *
 * Description: Condition that indicates when Vector's roll falls in between a certain range.
 *
 * Copyright: Anki, Inc. 2019
 *
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionRobotRollInRange.h"

#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"

namespace Anki {
namespace Vector {

namespace {
  const char* kMinRollThresholdKey = "minRollThreshold_deg";
  const char* kMaxRollThresholdKey = "maxRollThreshold_deg";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionRobotRollInRange::ConditionRobotRollInRange(const Json::Value& config)
: IBEICondition(config)
{
  const std::string& debugName = GetDebugLabel() + ".LoadConfig";
  _minRollThreshold_rad = DEG_TO_RAD(JsonTools::ParseFloat(config, kMinRollThresholdKey, debugName));
  _maxRollThreshold_rad = DEG_TO_RAD(JsonTools::ParseFloat(config, kMaxRollThresholdKey, debugName));
  const std::string& errorMsg = debugName + ".InvalidRollRangeThresholds";
  ANKI_VERIFY(_minRollThreshold_rad < _maxRollThreshold_rad, errorMsg.c_str(),
              "Minimum desired roll of %f [deg] is not less than maximum desired roll of %f [deg]",
              _minRollThreshold_rad.getDegrees(), _maxRollThreshold_rad.getDegrees());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionRobotRollInRange::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const auto& currRoll_rad = behaviorExternalInterface.GetRobotInfo().GetRollAngle();
  return currRoll_rad >= _minRollThreshold_rad && currRoll_rad <= _maxRollThreshold_rad;
}

} // namespace Vector
} // namespace Anki
