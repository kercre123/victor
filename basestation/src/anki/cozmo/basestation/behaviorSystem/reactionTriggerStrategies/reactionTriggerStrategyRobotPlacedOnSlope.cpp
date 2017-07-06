/**
 * File: ReactionTriggerStrategyRobotPlacedOnSlope.cpp
 *
 * Author: Matt Michini
 * Created: 01/25/17
 *
 * Description: Reaction Trigger strategy for responding to
 *
 * Copyright: Anki, Inc. 2017
 *
 *
 **/


#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyRobotPlacedOnSlope.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/behaviorSystem/wantsToRunStrategies/iWantsToRunStrategy.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/common/basestation/utils/timer.h"
#include "clad/types/robotStatusAndActions.h"

namespace Anki {
namespace Cozmo {
  
  
namespace{
static const char* kTriggerStrategyName = "Trigger Strategy Placed On Slope";
}
  
ReactionTriggerStrategyRobotPlacedOnSlope::ReactionTriggerStrategyRobotPlacedOnSlope(Robot& robot, const Json::Value& config)
: IReactionTriggerStrategy(robot, config, kTriggerStrategyName)
{
  
}

void ReactionTriggerStrategyRobotPlacedOnSlope::SetupForceTriggerBehavior(const Robot& robot, const IBehaviorPtr behavior)
{
  behavior->IsRunnable(ReactionTriggerConst::kNoPreReqs);
}
  
bool ReactionTriggerStrategyRobotPlacedOnSlope::ShouldTriggerBehaviorInternal(const Robot& robot, const IBehaviorPtr behavior)
{
  
  if(ANKI_VERIFY(_wantsToRunStrategy != nullptr,
                 "ReactionTriggerStrategyPlacedOnCharger.ShouldTriggerBehaviorInternal",
                 "WantsToRunStrategyNotSpecified")){
    return _wantsToRunStrategy->WantsToRun(robot) && behavior->IsRunnable(ReactionTriggerConst::kNoPreReqs);;
  }
  return false;
}

  
} // namespace Cozmo
} // namespace Anki
