/**
* File: ConditionFrustration.h
*
* Author:  Kevin M. Karol
* Created: 11/1/17
*
* Description: Strategy which indicates when Cozmo has become frustrated
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/beiConditions/conditions/conditionFrustration.h"

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/cozmoContext.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/robot.h"

#include "coretech/common/engine/utils/timer.h"

namespace Anki {
namespace Cozmo {
  
namespace{
static const char* kReactionConfigKey = "frustrationParams";
  
static const char* kMaxConfidenceKey = "maxConfidence";
static const char* kCooldownTime_sKey = "cooldownTime_s";
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionFrustration::ConditionFrustration(const Json::Value& config)
  : IBEICondition(config)
{
  LoadJson(config);  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionFrustration::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  float confidentVal = 0;
  if(behaviorExternalInterface.HasMoodManager()){
    auto& moodManager = behaviorExternalInterface.GetMoodManager();
    confidentVal = moodManager.GetEmotionValue(EmotionType::Confident);
  }
  
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  if(confidentVal < _maxConfidentScore &&
     ( (_lastReactedTime_s <= 0.f) || ((currTime_s - _lastReactedTime_s) > _cooldownTime_s)) ){
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionFrustration::LoadJson(const Json::Value& config)
{
  const Json::Value& frustrationParams = config[kReactionConfigKey];
  if(!frustrationParams.isNull()){
    _maxConfidentScore = frustrationParams.get(kMaxConfidenceKey, _maxConfidentScore).asFloat();
    _cooldownTime_s = frustrationParams.get(kCooldownTime_sKey, _cooldownTime_s).asFloat();
  }
}

} // namespace Cozmo
} // namespace Anki
