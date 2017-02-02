/**
 * File: reactionTriggerStrategyDoubleTap.h
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

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyDoubleTapDetected.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {
  
namespace{
static const char* kTriggerStrategyName = "Trigger Strategy Double Tap";

}
  
ReactionTriggerStrategyDoubleTapDetected::ReactionTriggerStrategyDoubleTapDetected(Robot& robot, const Json::Value& config)
:IReactionTriggerStrategy(robot, config, kTriggerStrategyName)
{
  
}


bool ReactionTriggerStrategyDoubleTapDetected::ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior)
{
  BehaviorPreReqRobot preReqData(robot);
  return behavior->IsRunnable(preReqData);
}


} // namespace Cozmo
} // namespace Anki
