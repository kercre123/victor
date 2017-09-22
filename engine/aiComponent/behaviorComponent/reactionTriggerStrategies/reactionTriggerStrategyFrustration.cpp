/**
 * File: reactionTriggerStrategyFrustration.cpp
 *
 * Author: Kevin M. Karol
 * Created: 12/08/16
 *
 * Description: Reaction Trigger strategy for responding to
 *
 * Copyright: Anki, Inc. 2016
 *
 *
 **/

#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerStrategyFrustration.h"

#include "engine/aiComponent/behaviorComponent/behaviors/iBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorManager.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/robot.h"
#include "engine/events/animationTriggerHelpers.h"
#include "anki/common/basestation/utils/timer.h"
#include "util/math/math.h"

namespace Anki {
namespace Cozmo {
  
namespace{
static const char* kReactionConfigKey = "frustrationParams";
static const char* kTriggerStrategyName = "Trigger Strategy Frustration";
  
static const char* kMaxConfidenceKey = "maxConfidence";
static const char* kCooldownTime_sKey = "cooldownTime_s";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ReactionTriggerStrategyFrustration::ReactionTriggerStrategyFrustration(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config)
: IReactionTriggerStrategy(behaviorExternalInterface, config, kTriggerStrategyName)
{
  LoadJson(config);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyFrustration::LoadJson(const Json::Value& config)
{
  const Json::Value& frustrationParams = config[kReactionConfigKey];
  if(!frustrationParams.isNull()){
    _maxConfidentScore = frustrationParams.get(kMaxConfidenceKey, _maxConfidentScore).asFloat();
    _cooldownTime_s = frustrationParams.get(kCooldownTime_sKey, _cooldownTime_s).asFloat();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyFrustration::SetupForceTriggerBehavior(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr behavior)
{
  behavior->WantsToBeActivated(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ReactionTriggerStrategyFrustration::ShouldTriggerBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr behavior)
{
  float confidentVal = 0;
  auto moodManager = behaviorExternalInterface.GetMoodManager().lock();
  if(moodManager != nullptr){
    confidentVal = moodManager->GetEmotionValue(EmotionType::Confident);
  }
  
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  const bool notReactingToFrustration =
   robot.GetBehaviorManager().GetCurrentReactionTrigger() != ReactionTrigger::Frustration;
  
  if(notReactingToFrustration &&
     confidentVal < _maxConfidentScore &&
     ( (_lastReactedTime_s <= 0.f) || ((currTime_s - _lastReactedTime_s) > _cooldownTime_s)) ){
    return behavior->WantsToBeActivated(behaviorExternalInterface);
  }
  
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyFrustration::BehaviorThatStrategyWillTriggerInternal(IBehaviorPtr behavior)
{
  behavior->AddListener(this);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyFrustration::AnimationComplete(BehaviorExternalInterface& behaviorExternalInterface)
{
  _lastReactedTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}

  
  
} // namespace Cozmo
} // namespace Anki
