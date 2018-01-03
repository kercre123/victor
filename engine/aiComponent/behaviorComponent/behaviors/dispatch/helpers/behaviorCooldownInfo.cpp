/**
 * File: behaviorCooldownInfo.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-10-31
 *
 * Description: Simple helper to track cooldowns
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/helpers/behaviorCooldownInfo.h"

#include "coretech/common/include/anki/common/basestation/jsonTools.h"
#include "coretech/common/include/anki/common/basestation/utils/timer.h"
#include "json/json.h"
#include "util/random/randomGenerator.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCooldownInfo::BehaviorCooldownInfo(float cooldownLength_s, float randomFactor)
  : _cooldown_s(cooldownLength_s)
  , _randomCooldownFactor(randomFactor)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCooldownInfo::BehaviorCooldownInfo(const Json::Value& config)
{
  const float cooldown = JsonTools::ParseFloat(config,
                                               "cooldown_s",
                                               "BehaviorCooldownInfo.Cooldown");
  const float randomFactor = config.get("cooldown_random_factor", 0.0f).asFloat();

  _cooldown_s = cooldown;
  _randomCooldownFactor = randomFactor;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorCooldownInfo::OnCooldown() const
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  return _onCooldownUntil_s >= 0.0f && currTime_s < _onCooldownUntil_s;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float BehaviorCooldownInfo::GetRemainingCooldownTime_s() const
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if( _onCooldownUntil_s < 0 || currTime_s >= _onCooldownUntil_s ) {
    return 0.0f;
  }
  return _onCooldownUntil_s - currTime_s;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCooldownInfo::StartCooldown(Util::RandomGenerator& rng)
{
  if( _cooldown_s > 0.0f ) {
    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    float cooldown_s = _cooldown_s;
    if( _randomCooldownFactor > 0.0f ) {
      cooldown_s *= rng.RandDblInRange( (1.0f - _randomCooldownFactor) * _cooldown_s,
                                        (1.0f + _randomCooldownFactor) * _cooldown_s );
    }
    
    _onCooldownUntil_s = currTime_s + cooldown_s;
  }
}

}
}
