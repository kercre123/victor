/**
 * File: reactionTriggerStrategyNoPreDockPoses.cpp
 *
 * Author: Kevin M. Karol
 * Created: 12/08/16
 *
 * Description: Reaction Trigger strategy for responding to an object with no predock poses
 *
 * Copyright: Anki, Inc. 2016
 *
 *
 **/

#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyNoPreDockPoses.h"

#include "engine/behaviorSystem/behaviors/iBehavior.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/behaviorSystem/behaviorPreReqs/behaviorPreReqAcknowledgeObject.h"
#include "engine/robot.h"

#include "anki/common/basestation/objectIDs.h"

namespace{
static const char* kTriggerStrategyName = "NoPreDockPoses";
}


namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ReactionTriggerStrategyNoPreDockPoses::ReactionTriggerStrategyNoPreDockPoses(Robot& robot, const Json::Value& config)
: IReactionTriggerStrategy(robot, config, kTriggerStrategyName)
{
}


void ReactionTriggerStrategyNoPreDockPoses::SetupForceTriggerBehavior(const Robot& robot, const IBehaviorPtr behavior)
{
  behavior->IsRunnable(ReactionTriggerConst::kNoPreReqs);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ReactionTriggerStrategyNoPreDockPoses::ShouldTriggerBehaviorInternal(const Robot& robot, const IBehaviorPtr behavior)
{
  const ObjectID& objID = robot.GetAIComponent().GetWhiteboard().GetNoPreDockPosesOnObject();
  if(objID.IsSet()){
    const ObjectID objectWithoutPosesCopy = objID;
    ObjectID unsetObj;
    robot.GetAIComponent().GetNonConstWhiteboard().SetNoPreDockPosesOnObject(unsetObj);
    
    BehaviorPreReqAcknowledgeObject preReq(objectWithoutPosesCopy, robot);
    return behavior->IsRunnable(preReq);
  }
  return false;
}

  
} // namespace Cozmo
} // namespace Anki
