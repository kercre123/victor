/**
 * File: ReactionTriggerStrategyRobotPickedUp.cpp
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


#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyRobotPickedUp.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

namespace{
static const char* kTriggerStrategyName = "Trigger Strategy Pickup";
}
  
ReactionTriggerStrategyRobotPickedUp::ReactionTriggerStrategyRobotPickedUp(Robot& robot, const Json::Value& config)
: IReactionTriggerStrategy(robot, config, kTriggerStrategyName)
{
  
}

  
bool ReactionTriggerStrategyRobotPickedUp::ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior)
{
  return robot.GetOffTreadsState() == OffTreadsState::InAir &&
                  behavior->IsRunnable(ReactionTriggerConst::kNoPreReqs);
}

  
} // namespace Cozmo
} // namespace Anki
