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

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "json/json.h"
#include "util/console/consoleInterface.h"
#include "util/math/math.h"
#include "util/random/randomGenerator.h"

namespace Anki {
namespace Vector {
  
CONSOLE_VAR_EXTERN(float, kTimeMultiplier);

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
  
  _ignoreFastForward = config.get("ignoreFastForward", false).asBool();

  if( Anki::Util::IsFltLTZero( cooldown ) ) { // a clearly negative cooldown ==> forever
    _cooldown_s = std::numeric_limits<float>::max();
    _randomCooldownFactor = 0.0f;
  } else {
    _cooldown_s = cooldown;
    _randomCooldownFactor = randomFactor;
  }
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
      cooldown_s *= rng.RandDblInRange( Anki::Util::Max(0.0f, 1.0f - _randomCooldownFactor),
                                        1.0f + _randomCooldownFactor );
    }
    
    if( !_ignoreFastForward ) {
      cooldown_s /= kTimeMultiplier;
    }
    _onCooldownUntil_s = currTime_s + cooldown_s;
  }
}

}
}
