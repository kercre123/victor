/**
 * File: conditionCliffDetected.cpp
 *
 * Author: Matt Michini
 * Created: 2018/02/21
 *
 * Description: Condition that indicates when victor stops at a detected cliff
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionCliffDetected.h"

#include "coretech/common/engine/jsonTools.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/sensors/cliffSensorComponent.h"

namespace Anki {
namespace Vector {
  
namespace {
  const char* kNumCliffsToTriggerKey = "numCliffDetectionsToTrigger";
}

ConditionCliffDetected::ConditionCliffDetected(const Json::Value& config)
: IBEICondition(config)
{
  // Check whether the user specified a specific number of cliff sensors detecting
  // a cliff in order to trigger this condition
  if (JsonTools::GetValueOptional(config, kNumCliffsToTriggerKey, _numCliffDetectionsToTrigger)) {
    const std::string& errorName = GetDebugLabel() + ".LoadConfig.InvalidNumCliffDetections";
    // Check for negative or zero values in the config by casting to a signed integer first.
    const int8_t tempInt = JsonTools::ParseInt8(config, kNumCliffsToTriggerKey, errorName);
    ANKI_VERIFY(tempInt > 0, errorName.c_str(),
                "Number of cliff detections to trigger, %d, must be a positive value (>0).",
                tempInt);
    ANKI_VERIFY(_numCliffDetectionsToTrigger <= CliffSensorComponent::kNumCliffSensors,
                errorName.c_str(),
                "Number of cliff detections to trigger, %d, exceeds number of cliff sensors, %d",
                _numCliffDetectionsToTrigger, CliffSensorComponent::kNumCliffSensors);
  } else {
    _numCliffDetectionsToTrigger = 1;
  }
}


bool ConditionCliffDetected::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const auto& cliffSensorComponent = behaviorExternalInterface.GetRobotInfo().GetCliffSensorComponent();
  
  if (_numCliffDetectionsToTrigger <= 1) {
    return cliffSensorComponent.IsCliffDetected();
  } else {
    // Count the number of cliff detections reported by all cliff sensors on the robot, and
    // report whether the number exceeds the threshold set by the caller/creator of the condition.
    u8 cliffsDetected = 0;
    for (int i = 0; i < CliffSensorComponent::kNumCliffSensors; ++i) {
      if (cliffSensorComponent.IsCliffDetected((CliffSensor)(i))) {
        ++cliffsDetected;
        if (cliffsDetected >= _numCliffDetectionsToTrigger) {
          return true;
        } 
      }
    }
    return false;
  }
}


} // namespace Vector
} // namespace Anki
