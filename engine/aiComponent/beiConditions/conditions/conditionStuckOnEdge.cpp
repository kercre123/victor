/**
 * File: conditionStuckOnEdge.cpp
 *
 * Author: Matt Michini
 * Created: 2018/02/21
 *
 * Description: Condition that indicates when victor is stuck on a table edge
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionStuckOnEdge.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/sensors/cliffSensorComponent.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "coretech/common/engine/utils/timer.h"

namespace Anki {
namespace Vector {

ConditionStuckOnEdge::ConditionStuckOnEdge(const Json::Value& config)
: IBEICondition(config) 
{
}


void ConditionStuckOnEdge::InitInternal(BehaviorExternalInterface& behaviorExternalInterface) 
{
  _onEdgeStartTime_s = 0.f;
}

bool ConditionStuckOnEdge::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
  auto& cliffComp = robotInfo.GetCliffSensorComponent();
  const bool isPickedUp = robotInfo.IsPickedUp();

  static const uint8_t kLeftCliffSensors  = (1 << static_cast<int>(CliffSensor::CLIFF_FL)) | (1 << static_cast<int>(CliffSensor::CLIFF_BL));
  static const uint8_t kRightCliffSensors = (1 << static_cast<int>(CliffSensor::CLIFF_FR)) | (1 << static_cast<int>(CliffSensor::CLIFF_BR));
  const bool stuckOnEdge = cliffComp.GetCliffDetectedFlags() == kLeftCliffSensors ||
                           cliffComp.GetCliffDetectedFlags() == kRightCliffSensors;

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  static const float latencyDelay_s = 0.05f;
  static const float maxCancelByPickupTime_s = Util::MilliSecToSec((float)CLIFF_EVENT_DELAY_MS) + latencyDelay_s;

  // Return true if condition has held long enough to not turn out
  // to be due to pickup.
  if (stuckOnEdge) {
    if (isPickedUp) {
      _onEdgeStartTime_s = 0.f;
    } else if (NEAR_ZERO(_onEdgeStartTime_s)) {
      _onEdgeStartTime_s = currTime_s;
    } else if (currTime_s - _onEdgeStartTime_s > maxCancelByPickupTime_s) {
      return true;
    }
  }

  return false;
}


} // namespace Vector
} // namespace Anki
