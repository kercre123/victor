/**
 * File: behaviorReactToPoke.cpp
 *
 * Author: Kevin
 * Created: 11/30/15
 *
 * Description: Behavior for immediately responding to being poked
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorReactToPoke.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;
  
BehaviorReactToPoke::BehaviorReactToPoke(Robot& robot, const Json::Value& config)
: IReactionaryBehavior(robot, config)
{
  
  SetDefaultName("ReactToPoke");
  
  // These are the tags that should trigger this behavior to be switched to immediately
  SubscribeToTriggerTags({
    EngineToGameTag::RobotPoked
  });
  
  // These are additional tags that this behavior should handle
  SubscribeToTags({
    EngineToGameTag::RobotCompletedAction
  });
  
}

bool BehaviorReactToPoke::IsRunnableInternalReactionary(const Robot& robot) const
{
  return true;
}

Result BehaviorReactToPoke::InitInternalReactionary(Robot& robot)
{
  robot.GetActionList().Cancel(RobotActionType::TRACK_FACE);
  robot.GetActionList().Cancel(RobotActionType::TRACK_OBJECT);
  return Result::RESULT_OK;
}

IBehavior::Status BehaviorReactToPoke::UpdateInternal(Robot& robot)
{
  const double currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  if (_isActing) {
    return Status::Running;
  }
  
  switch (_currentState)
  {
    case State::Inactive:
    {
      if (_doReaction)
      {
        _currentState = State::ReactToPoke;
        break;
      }
      return Status::Complete;
    }
    case State::ReactToPoke:
    {
      // Decrease happy
      robot.GetMoodManager().AddToEmotion(EmotionType::Happy, -kEmotionChangeLarge, "Poked", currentTime_sec);
      
      // Do startled animation
      StartActing(robot, new TriggerAnimationAction(robot, AnimationTrigger::ReactToPokeStartled));
      _currentState = State::TurnToFace;
      break;
    }
    case State::TurnToFace:
    {
      // Find most recently observed face and turn to it
      Pose3d facePose;
      TimeStamp_t lastObservedFaceTime = robot.GetFaceWorld().GetLastObservedFace(facePose);
      if (lastObservedFaceTime > 0 &&
          (robot.GetLastMsgTimestamp() - lastObservedFaceTime < kMaxTimeSinceLastObservedFace_ms)) {
        PRINT_NAMED_INFO("BehaviorReactToPoke.TurnToFace.TurningToLastObservedFace","time = %d", lastObservedFaceTime);

        TurnTowardsPoseAction* action = new TurnTowardsPoseAction(robot, facePose, DEG_TO_RAD(180));
        action->SetPanTolerance(DEG_TO_RAD(3));
        action->SetMaxPanSpeed(DEG_TO_RAD(1080));
        action->SetMaxTiltSpeed(DEG_TO_RAD(1080));
        action->SetPanAccel(200);
        action->SetTiltAccel(200);
        StartActing(robot, action);
      }
      _currentState = State::ReactToFace;
      break;
    }
    case State::ReactToFace:
    {
      StartActing(robot, new TriggerAnimationAction(robot, AnimationTrigger::ReactToPokeReaction));
      _currentState = State::Inactive;
      _doReaction = false;
      
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("BehaviorReactToPoke.Update.UnknownState",
                        "Reached unknown state %d.", _currentState);
    }
  }
  
  return Status::Running;
}

void BehaviorReactToPoke::StopInternalReactionary(Robot& robot)
{
}

void BehaviorReactToPoke::AlwaysHandleInternal(const EngineToGameEvent& event,
                                         const Robot& robot)
{

// this behavior is currently not being used. If we reenable it, we may need to revisit why this reaction
// needed to be disabled in the first place
//  if( ! IsChoosable() ) {
//    return;
//  }

  // We want to get these messages, even when not running
  switch (event.GetData().GetTag())
  {
    case MessageEngineToGameTag::RobotPoked:
    {
      _doReaction = true;
      break;
    }
    case MessageEngineToGameTag::RobotCompletedAction:
    {
      const RobotCompletedAction& msg = event.GetData().Get_RobotCompletedAction();
      if (_lastActionTag == msg.idTag)
      {
        _isActing = false;
      }
      break;
    }
    default:
    {
      break;
    }
  }
}

  
  
  void BehaviorReactToPoke::StartActing(Robot& robot, IActionRunner* action)
  {
    
    _lastActionTag = action->GetTag();
    robot.GetActionList().QueueActionAtEnd(action);
    _isActing = true;
  }

  
} // namespace Cozmo
} // namespace Anki
