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
#include "engine/components/powerStateManager.h"
#include "engine/components/sensors/cliffSensorComponent.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "coretech/common/engine/utils/timer.h"

namespace Anki {
namespace Vector {

namespace {
  const uint8_t FL = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FL));
  const uint8_t FR = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FR));
  const uint8_t BL = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_BL));
  const uint8_t BR = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_BR));
  const uint8_t kLeftCliffSensors = FL | BL;
  const uint8_t kRightCliffSensors = FR | BR;
  
  // Delay to prevent false-positive detection due to pick-up event.
  // Accounts for cliff sensor latency.
  const float kLatencyDelay_s = 0.05f;
  const float kMaxCancelByPickupTime_s = Util::MilliSecToSec((float)CLIFF_EVENT_DELAY_MS) + kLatencyDelay_s;
}

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
  const auto& powerSaveManager = behaviorExternalInterface.GetPowerStateManager();
  const bool inSysconCalmMode = powerSaveManager.InSysconCalmMode();
  
  auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
  const bool isPickedUp = robotInfo.IsPickedUp();
  
  // We can only check the cliff sensor components if Syscon is not in Calm Mode.
  if (!inSysconCalmMode) {
    auto& cliffComp = robotInfo.GetCliffSensorComponent();
    const uint8_t cliffFlags = cliffComp.GetCliffDetectedFlags();
    // Robot is considered stuck iff two cliff sensors on the same side
    // or on the same diagonal are detecting cliffs.
    const bool stuckOnEdge = cliffFlags == kLeftCliffSensors ||
                             cliffFlags == kRightCliffSensors ||
                             cliffFlags == (BL | FR) ||
                             cliffFlags == (BR | FL);

    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

    // Return true if condition has held long enough to not turn out
    // to be due to pickup.
    if (stuckOnEdge) {
      if (isPickedUp) {
        _onEdgeStartTime_s = 0.f;
      } else if (NEAR_ZERO(_onEdgeStartTime_s)) {
        _onEdgeStartTime_s = currTime_s;
      } else if (currTime_s - _onEdgeStartTime_s > kMaxCancelByPickupTime_s) {
        return true;
      }
    }
  }
  
  return false;
}


} // namespace Vector
} // namespace Anki
