/**
 * File: behaviorCooldownInfo.h
 *
 * Author: Brad Neuman
 * Created: 2017-10-31
 *
 * Description: Simple helper to track cooldowns
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_Dispatch_Helpers_BehaviorCooldownInfo_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_Dispatch_Helpers_BehaviorCooldownInfo_H__

#include "json/json-forwards.h"

namespace Anki {

namespace Util {
class RandomGenerator;
}

namespace Cozmo {

class BehaviorCooldownInfo
{
public:
  
  explicit BehaviorCooldownInfo(float cooldownLength_s, float randomFactor = 0.0f);
  explicit BehaviorCooldownInfo(const Json::Value& config);
  
  bool OnCooldown() const;
  void StartCooldown(Util::RandomGenerator& rng);
  
  void ResetCooldown() { _onCooldownUntil_s = -1.0f; }

  // if on cooldown, return the remaining seconds of cooldown, else return 0
  float GetRemainingCooldownTime_s() const;

private:
  // params
  float _cooldown_s = 0.0f;;
  float _randomCooldownFactor = 0.0f;

  // members
  float _onCooldownUntil_s = -1.0f;
  
  // Whether or not to ignore the dev-only time-speeder-upper (kTimeMultiplier)
  bool _ignoreFastForward = false;
};

}
}


#endif
