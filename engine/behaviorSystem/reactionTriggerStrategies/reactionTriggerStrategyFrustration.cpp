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

#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyFrustration.h"

#include "engine/behaviorSystem/behaviors/iBehavior.h"
#include "engine/behaviorSystem/behaviorManager.h"
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
  
ReactionTriggerStrategyFrustration::ReactionTriggerStrategyFrustration(Robot& robot, const Json::Value& config)
: IReactionTriggerStrategy(robot, config, kTriggerStrategyName)
{
  LoadJson(config);
}

void ReactionTriggerStrategyFrustration::LoadJson(const Json::Value& config)
{
  const Json::Value& frustrationParams = config[kReactionConfigKey];
  if(!frustrationParams.isNull()){
    _maxConfidentScore = frustrationParams.get(kMaxConfidenceKey, _maxConfidentScore).asFloat();
    _cooldownTime_s = frustrationParams.get(kCooldownTime_sKey, _cooldownTime_s).asFloat();
  }
}

void ReactionTriggerStrategyFrustration::SetupForceTriggerBehavior(const Robot& robot, const IBehaviorPtr behavior)
{
  behavior->IsRunnable(ReactionTriggerConst::kNoPreReqs);
}
  
bool ReactionTriggerStrategyFrustration::ShouldTriggerBehaviorInternal(const Robot& robot, const IBehaviorPtr behavior)
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  float confidentVal = robot.GetMoodManager().GetEmotionValue(EmotionType::Confident);
  
  const bool notReactingToFrustration =
   robot.GetBehaviorManager().GetCurrentReactionTrigger() != ReactionTrigger::Frustration;
  
  if(notReactingToFrustration &&
     confidentVal < _maxConfidentScore &&
     ( (_lastReactedTime_s <= 0.f) || ((currTime_s - _lastReactedTime_s) > _cooldownTime_s)) ){
    return behavior->IsRunnable(ReactionTriggerConst::kNoPreReqs);
  }
  
  return false;
}

void ReactionTriggerStrategyFrustration::BehaviorThatStrategyWillTriggerInternal(IBehaviorPtr behavior)
{
  behavior->AddListener(this);
}

void ReactionTriggerStrategyFrustration::AnimationComplete(Robot& robot)
{
  _lastReactedTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}

  
  
} // namespace Cozmo
} // namespace Anki
