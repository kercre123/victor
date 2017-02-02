/**
 * File: reactionTriggerStrategyRobotOnBack.cpp
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

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyRobotOnBack.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {
  
namespace{
static const char* kTriggerStrategyName = "Trigger strategy robot on back";
}

  
ReactionTriggerStrategyRobotOnBack::ReactionTriggerStrategyRobotOnBack(Robot& robot, const Json::Value& config)
: IReactionTriggerStrategy(robot, config, kTriggerStrategyName)
{
}
  

bool ReactionTriggerStrategyRobotOnBack::ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior)
{
  return robot.GetOffTreadsState() == OffTreadsState::OnBack &&
           behavior->IsRunnable(ReactionTriggerConst::kNoPreReqs);
}

} // namespace Cozmo
} // namespace Anki
