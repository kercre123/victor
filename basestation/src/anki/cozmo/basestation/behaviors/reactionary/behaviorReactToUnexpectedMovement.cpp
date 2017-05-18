/**
 * File: behaviorReactToUnexpectedMovement.cpp
 *
 * Author: Al Chaussee
 * Created: 7/11/2016
 *
 * Description: Behavior for reacting to unexpected movement like being spun while moving
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToUnexpectedMovement.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"


namespace Anki {
namespace Cozmo {
  
BehaviorReactToUnexpectedMovement::BehaviorReactToUnexpectedMovement(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{  
  SubscribeToTags({
    EngineToGameTag::UnexpectedMovement
  });
}

bool BehaviorReactToUnexpectedMovement::IsRunnableInternal(const BehaviorPreReqNone& preReqData) const
{
  return true;
}

Result BehaviorReactToUnexpectedMovement::InitInternal(Robot& robot)
{
  // Make Cozmo more frustrated if he keeps running into things/being turned
  robot.GetMoodManager().TriggerEmotionEvent("ReactToUnexpectedMovement", MoodManager::GetCurrentTimeInSeconds());
  
  StartActing(new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::ReactToUnexpectedMovement), [this](){
    BehaviorObjectiveAchieved(BehaviorObjective::ReactedToUnexpectedMovement);
  });
  
  return RESULT_OK;
}
  
}
}
