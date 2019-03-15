/**
 * File: conditionRobotHeldInPalm.cpp
 *
 * Author: Guillermo Bautista
 * Created: 2019/03/11
 *
 * Description: Condition that checks if Vector's held-in-palm matches that of the supplied config,
 * with the option to specify a minimum and/or maximum duration of time that Vector must be held in
 * the user's palm.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionRobotHeldInPalm.h"

#include "coretech/common/engine/jsonTools.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/heldInPalmTracker.h"

#include "util/math/math.h"

namespace Anki {
namespace Vector {
  
namespace {
  const char* kShouldBeHeldInPalmKey  = "shouldBeHeldInPalm";
  // Optional configuration keys
  const char* kMinDurationKey           = "minDuration_s";
  const char* kMaxDurationKey           = "maxDuration_s";
}

ConditionRobotHeldInPalm::ConditionRobotHeldInPalm(const Json::Value& config)
: IBEICondition(config)
{
  _shouldBeHeldInPalm = JsonTools::ParseBool(config, kShouldBeHeldInPalmKey,
                                             "ConditionRobotHeldInPalm.Config");
  _minDuration_ms = Util::SecToMilliSec(config.get(kMinDurationKey, 0.0).asFloat());
  _maxDuration_ms = Util::SecToMilliSec(config.get(kMaxDurationKey, FLT_MAX).asFloat());
}

ConditionRobotHeldInPalm::ConditionRobotHeldInPalm(const bool shouldBeHeldInPalm,
                                                   const std::string& ownerDebugLabel,
                                                   const int minDuration_ms,
                                                   const int maxDuration_ms)
  : IBEICondition(IBEICondition::GenerateBaseConditionConfig(BEIConditionType::RobotHeldInPalm))
  , _shouldBeHeldInPalm(shouldBeHeldInPalm)
  , _minDuration_ms(minDuration_ms)
  , _maxDuration_ms(maxDuration_ms)
{
  SetOwnerDebugLabel(ownerDebugLabel);
}

bool ConditionRobotHeldInPalm::AreConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  const auto& heldInPalmTracker = bei.GetHeldInPalmTracker();
  if ( heldInPalmTracker.IsHeldInPalm() == _shouldBeHeldInPalm ) {
    if (_shouldBeHeldInPalm) {
      const u32 heldInPalmDuration_ms = heldInPalmTracker.GetHeldInPalmDuration_ms();
      return heldInPalmDuration_ms >= _minDuration_ms && heldInPalmDuration_ms <= _maxDuration_ms;
    } else {
      const u32 notHeldInPalmDuration_ms = heldInPalmTracker.GetTimeSinceLastHeldInPalm_ms();
      return notHeldInPalmDuration_ms >= _minDuration_ms && notHeldInPalmDuration_ms <= _maxDuration_ms;
    }
  }
  return false;
}


} // namespace Vector
} // namespace Anki
