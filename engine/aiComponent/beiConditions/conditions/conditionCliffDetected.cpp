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
  // Optional configuration keys
  const char* kNumCliffsToTriggerKey    = "numCliffDetectionsToTrigger";
  const char* kShouldDetectNoCliffsKey  = "shouldDetectNoCliffs";
  const char* kMinDurationKey           = "minDuration_ms";
  const char* kMaxDurationKey           = "maxDuration_ms";
}

ConditionCliffDetected::ConditionCliffDetected(const Json::Value& config)
: IBEICondition(config)
{
  // Check whether the user specified a minimum number of cliff sensors detecting
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
  
  if (JsonTools::GetValueOptional(config, kShouldDetectNoCliffsKey, _shouldDetectNoCliffs)) {
    const std::string& errorName = GetDebugLabel() + ".LoadConfig.InvalidNoCliffsKey";
    ANKI_VERIFY(!config.isMember(kNumCliffsToTriggerKey), errorName.c_str(),
                "%s key already specified in configuration, cannot override with %s modifier",
                kNumCliffsToTriggerKey,
                kShouldDetectNoCliffsKey);
    _numCliffDetectionsToTrigger = _shouldDetectNoCliffs ? 0 : 1;
  } else {
    _shouldDetectNoCliffs = false;
  }
  
  _minDuration_ms = config.get(kMinDurationKey, 0).asInt();
  _maxDuration_ms = config.get(kMaxDurationKey, INT_MAX).asInt();
}


bool ConditionCliffDetected::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  const auto& cliffSensorComponent = behaviorExternalInterface.GetRobotInfo().GetCliffSensorComponent();
  const int numCliffs = cliffSensorComponent.GetNumCliffsDetected();
  if ( (_shouldDetectNoCliffs && numCliffs == 0) ||
       (!_shouldDetectNoCliffs && numCliffs >= _numCliffDetectionsToTrigger) ) {
    const int currDetectionDuration = cliffSensorComponent.GetDurationForAtLeastNCliffDetections_ms(_numCliffDetectionsToTrigger);
    return (currDetectionDuration >= _minDuration_ms) && (currDetectionDuration <= _maxDuration_ms);
  }
  return false;
}


} // namespace Vector
} // namespace Anki
