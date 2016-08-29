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
: IReactionaryBehavior(robot, config)
{
  SetDefaultName("ReactToUnexpectedMovement");
  
  SubscribeToTriggerTags({
    EngineToGameTag::UnexpectedMovement
  });
}

bool BehaviorReactToUnexpectedMovement::IsRunnableInternalReactionary(const Robot& robot) const
{
  return true;
}

Result BehaviorReactToUnexpectedMovement::InitInternalReactionary(Robot& robot)
{
  // Make Cozmo more frustrated if he keeps running into things/being turned
  robot.GetMoodManager().TriggerEmotionEvent("ReactToUnexpectedMovement", MoodManager::GetCurrentTimeInSeconds());
  
  robot.AbortDrivingToPose();
  robot.GetMoveComponent().StopAllMotors();
  robot.GetActionList().Cancel();

  StartActing(new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::ReactToUnexpectedMovement), [this](){
    BehaviorObjectiveAchieved(BehaviorObjective::ReactedToUnexpectedMovement);
  });
  
  return RESULT_OK;
}

bool BehaviorReactToUnexpectedMovement::ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event, const Robot& robot)
{
  switch(event.GetTag())
  {
    case EngineToGameTag::UnexpectedMovement:
    {
      if(!IsRunning())
      {
        return true;
      }
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("BehaviorReactToUnexpectedMovement.ShouldRunForEvent.BadEventType",
                        "Calling ShouldRunForEvent with an event we don't care about, this is a bug");
      break;
    }
  }
  return false;
}

  
}
}