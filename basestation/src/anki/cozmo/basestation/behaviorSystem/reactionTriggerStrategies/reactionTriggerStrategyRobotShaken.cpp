/**
 * File: reactionTriggerStrategyRobotShaken.cpp
 *
 * Author: Matt Michini
 * Created: 2017/01/11
 *
 * Description: Reaction Trigger strategy for responding to robot being shaken
 *
 * Copyright: Anki, Inc. 2017
 *
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyRobotShaken.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {
  
namespace{
static const char* kTriggerStrategyName = "Trigger strategy robot shaken";
}

const float ReactionTriggerStrategyRobotShaken::_kAccelMagnitudeShakingStartedThreshold = 16000.f;
  
ReactionTriggerStrategyRobotShaken::ReactionTriggerStrategyRobotShaken(Robot& robot, const Json::Value& config)
: IReactionTriggerStrategy(robot, config, kTriggerStrategyName)
{
}
  
void ReactionTriggerStrategyRobotShaken::SetupForceTriggerBehavior(const Robot& robot, const IBehavior* behavior)
{
  behavior->IsRunnable(ReactionTriggerConst::kNoPreReqs);
}
  
bool ReactionTriggerStrategyRobotShaken::ShouldTriggerBehaviorInternal(const Robot& robot, const IBehavior* behavior)
{
  // ensure behavior is runnable (it is if already running - otherwise IsRunnable will assert):
  const bool isRunnable = behavior->IsRunning() || behavior->IsRunnable(ReactionTriggerConst::kNoPreReqs);
  
  // trigger this behavior when the filtered total accelerometer magnitude data exceeds a threshold
  bool shouldTrigger = (robot.GetHeadAccelMagnitudeFiltered() > _kAccelMagnitudeShakingStartedThreshold);
  
  // add a check for offTreadsState?
  
  return (shouldTrigger && isRunnable);
}

} // namespace Cozmo
} // namespace Anki
