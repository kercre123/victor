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
#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/behaviorSystem/wantsToRunStrategies/iWantsToRunStrategy.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {
  
namespace{
static const char* kTriggerStrategyName = "Trigger strategy robot shaken";
}
  
ReactionTriggerStrategyRobotShaken::ReactionTriggerStrategyRobotShaken(Robot& robot, const Json::Value& config)
: IReactionTriggerStrategy(robot, config, kTriggerStrategyName)
{
}
  
void ReactionTriggerStrategyRobotShaken::SetupForceTriggerBehavior(const Robot& robot, const IBehaviorPtr behavior)
{
  behavior->IsRunnable(ReactionTriggerConst::kNoPreReqs);
}
  
bool ReactionTriggerStrategyRobotShaken::ShouldTriggerBehaviorInternal(const Robot& robot, const IBehaviorPtr behavior)
{
  if(ANKI_VERIFY(_wantsToRunStrategy != nullptr,
                 "ReactionTriggerStrategyNoPreDockPoses.ShouldTriggerBehaviorInternal",
                 "WantsToRunStrategyNotSpecified")){
    const bool isRunnable = behavior->IsRunning() || behavior->IsRunnable(ReactionTriggerConst::kNoPreReqs);

    return _wantsToRunStrategy->WantsToRun(robot) && isRunnable;
  }
  
  return false;
}

} // namespace Cozmo
} // namespace Anki
