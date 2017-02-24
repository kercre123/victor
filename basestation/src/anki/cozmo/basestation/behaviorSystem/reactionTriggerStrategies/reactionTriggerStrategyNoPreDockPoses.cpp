/**
 * File: reactionTriggerStrategyNoPreDockPoses.cpp
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

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyNoPreDockPoses.h"

#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqAcknowledgeObject.h"
#include "anki/cozmo/basestation/robot.h"

namespace{
static const char* kTriggerStrategyName = "NoPreDockPoses";
}


namespace Anki {
namespace Cozmo {
  
//////
/// ReactAcknowledge Cube Moved
/////
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ReactionTriggerStrategyNoPreDockPoses::ReactionTriggerStrategyNoPreDockPoses(Robot& robot, const Json::Value& config)
: IReactionTriggerStrategy(robot, config, kTriggerStrategyName)
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ReactionTriggerStrategyNoPreDockPoses::ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior)
{
  const ObjectID& objID = robot.GetAIComponent().GetWhiteboard().GetNoPreDockPosesOnObject();
  if(objID.IsSet()){
    BehaviorPreReqAcknowledgeObject preReqData(objID.GetValue());
    ObjectID unsetObj;
    robot.GetAIComponent().GetNonConstWhiteboard().SetNoPreDockPosesOnObject(unsetObj);
    return behavior->IsRunnable(preReqData);
  }
  return false;
}

  
} // namespace Cozmo
} // namespace Anki
