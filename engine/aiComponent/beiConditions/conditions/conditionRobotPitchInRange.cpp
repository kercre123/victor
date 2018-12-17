/**
 * File: conditionRobotPitchInRange.cpp
 *
 * Author: Guillermo Bautista
 * Created: 2018/12/14
 *
 * Description: Condition that indicates when Vector's pitch falls in between a certain range.
 *
 * Copyright: Anki, Inc. 2018
 *
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionRobotPitchInRange.h"

#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"

namespace Anki {
namespace Vector {

namespace {
  const char* kMinPitchThresholdKey = "minPitchThreshold_deg";
  const char* kMaxPitchThresholdKey = "maxPitchThreshold_deg";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionRobotPitchInRange::ConditionRobotPitchInRange(const Json::Value& config)
: IBEICondition(config)
{
  const std::string& debugName = GetDebugLabel() + ".LoadConfig";
  _minPitchThreshold_rad = DEG_TO_RAD(JsonTools::ParseFloat(config, kMinPitchThresholdKey, debugName));
  _maxPitchThreshold_rad = DEG_TO_RAD(JsonTools::ParseFloat(config, kMaxPitchThresholdKey, debugName));
  const std::string& errorMsg = debugName + ".InvalidPitchRangeThresholds";
  ANKI_VERIFY(_minPitchThreshold_rad < _maxPitchThreshold_rad, errorMsg.c_str(),
              "Minimum desired pitch of %f [deg] is not less than maximum desired pitch of %f [deg]",
              _minPitchThreshold_rad.getDegrees(), _maxPitchThreshold_rad.getDegrees());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionRobotPitchInRange::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const auto& currPitch_rad = behaviorExternalInterface.GetRobotInfo().GetPitchAngle();
  return currPitch_rad >= _minPitchThreshold_rad && currPitch_rad <= _maxPitchThreshold_rad;
}

} // namespace Vector
} // namespace Anki
