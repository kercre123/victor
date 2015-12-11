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
#include "anki/cozmo/basestation/cozmoActions.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;
  
// Randomly plays an animation from here after first being poked
static std::vector<std::string> _animStartled = {
  "ID_pokedReaction_01"
};

// Plays one of these animations after facing a person.
// Animations are ordered from happy to irritated.
static std::vector<std::string> _animReactions = {
  "ID_poked_giggle",
  "ID_pokedA",
  "ID_pokedB"
};

// Thresholds for when to play which of the animReactions
static std::vector<f32> _animReactionHappyThresholds = {
  -0.1f,
  -0.8f
};
  
BehaviorReactToPoke::BehaviorReactToPoke(Robot& robot, const Json::Value& config)
: IReactionaryBehavior(robot, config)
{
  assert(_animReactions.size() == _animReactionHappyThresholds.size() + 1);
  
  _name = "ReactToPoke";
  
  // These are the tags that should trigger this behavior to be switched to immediately
  SubscribeToTriggerTags({
    EngineToGameTag::RobotPoked
  });
  
  // These are additional tags that this behavior should handle
  SubscribeToTags({
    EngineToGameTag::RobotCompletedAction
  });
  
}

bool BehaviorReactToPoke::IsRunnable(const Robot& robot, double currentTime_sec) const
{
  return true;
}

Result BehaviorReactToPoke::InitInternal(Robot& robot, double currentTime_sec, bool isResuming)
{
  robot.GetMoveComponent().DisableTrackToFace();
  robot.GetMoveComponent().DisableTrackToObject();
  return Result::RESULT_OK;
}

IBehavior::Status BehaviorReactToPoke::UpdateInternal(Robot& robot, double currentTime_sec)
{
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
      robot.GetMoodManager().AddToEmotion(EmotionType::Happy, -kEmotionChangeMedium, "Poked");
      
      // Do startled animation
      s32 animIndex = robot.GetLastMsgTimestamp() % _animStartled.size(); // Randomly select anim to play
      StartActing(robot, new PlayAnimationAction(_animStartled[animIndex]));
      _currentState = State::TurnToFace;
      break;
    }
    case State::TurnToFace:
    {
      // Find most recently observed face and turn to it
      Pose3d facePose;
      TimeStamp_t lastObservedFaceTime = robot.GetFaceWorld().GetLastObservedFace(facePose);
      if (lastObservedFaceTime > 0) {
        PRINT_NAMED_INFO("BehaviorReactToPoke.TurnToFace.TurningToLastObservedFace","time = %d", lastObservedFaceTime);
        FacePoseAction* action = new FacePoseAction(facePose, DEG_TO_RAD(3), DEG_TO_RAD(180));
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
      // Get happy reading
      f32 happyVal = robot.GetMoodManager().GetEmotionValue(EmotionType::Happy);
      
      // Figure out which reaction to play
      u32 animIndex = 0;
      for (; animIndex < _animReactionHappyThresholds.size(); ++animIndex) {
        if (happyVal > _animReactionHappyThresholds[animIndex]) {
          break;
        }
      }
      PRINT_NAMED_INFO("BehaviorReactToPoke.Update.HappyVal", "happy: %f, animIndex: %d", happyVal, animIndex);
      
      
      StartActing(robot, new PlayAnimationAction(_animReactions[animIndex]));
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
  
Result BehaviorReactToPoke::InterruptInternal(Robot& robot, double currentTime_sec, bool isShortInterrupt)
{
  // We don't want to be interrupted unless we're done reacting
  if (State::Inactive != _currentState)
  {
    return Result::RESULT_FAIL;
  }
  return Result::RESULT_OK;
}

void BehaviorReactToPoke::AlwaysHandle(const EngineToGameEvent& event,
                                         const Robot& robot)
{
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
    robot.GetActionList().QueueActionAtEnd(Robot::DriveAndManipulateSlot, action);
    _isActing = true;
  }

  
} // namespace Cozmo
} // namespace Anki