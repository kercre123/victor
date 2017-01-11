/**
 * File: reactionTriggerStrategyRobotOnSide.cpp
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

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyRobotOnSide.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/robot.h"


namespace Anki {
namespace Cozmo {

namespace{
static const char* kTriggerStrategyName = "Trigger Strategy Robot On Side";
}
  
ReactionTriggerStrategyRobotOnSide::ReactionTriggerStrategyRobotOnSide(Robot& robot, const Json::Value& config)
: Anki::Cozmo::IReactionTriggerStrategy(robot, config, kTriggerStrategyName)
{
  
}


bool ReactionTriggerStrategyRobotOnSide::ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior)
{
  return (robot.GetOffTreadsState() == OffTreadsState::OnLeftSide
           || robot.GetOffTreadsState() == OffTreadsState::OnRightSide) &&
           behavior->IsRunnable(ReactionTriggerConst::kNoPreReqs);
}


} // namespace Cozmo
} // namespace Anki
