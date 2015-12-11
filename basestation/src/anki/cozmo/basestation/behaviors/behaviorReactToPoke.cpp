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
  
  
static std::vector<f32> _animReactionHappyThresholds = {
  -0.1f,
  -0.8f
};
  
static std::vector<std::string> _animReactions = {
  "ID_poked_giggle",
  "ID_pokedA",
  "ID_pokedB"
};


BehaviorReactToPoke::BehaviorReactToPoke(Robot& robot, const Json::Value& config)
: IReactionaryBehavior(robot, config)
{
  assert(_animReactions.size() == _animReactionHappyThresholds.size() + 1);
  
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

bool BehaviorReactToPoke::IsRunnable(const Robot& robot, double currentTime_sec) const
{
  switch (_currentState)
  {
    case State::Inactive:
    case State::IsPoked:
    case State::PlayingAnimation:
    {
      return true;
    }
    default:
    {
      PRINT_NAMED_ERROR("BehaviorReactToPoke.IsRunnable.UnknownState",
                        "Reached unknown state %d.", _currentState);
    }
  }
  return false;
}

Result BehaviorReactToPoke::InitInternal(Robot& robot, double currentTime_sec, bool isResuming)
{
  return Result::RESULT_OK;
}

IBehavior::Status BehaviorReactToPoke::UpdateInternal(Robot& robot, double currentTime_sec)
{
  switch (_currentState)
  {
    case State::Inactive:
    {
      if (_doReaction)
      {
        _currentState = State::IsPoked;
        return Status::Running;
      }
      break; // Jump down and return Status::Complete
    }
    case State::IsPoked:
    {
      // For now we simply rotate through the animations we want to play when poked
      if (!_animReactions.empty())
      {
        // Decrease happy
        robot.GetMoodManager().AddToEmotion(EmotionType::Happy, -kEmotionChangeMedium, "Poked");
        
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
        IActionRunner* newAction = new PlayAnimationAction(_animReactions[animIndex]);
        _animTagToWaitFor = newAction->GetTag();
        robot.GetActionList().QueueActionNow(0, newAction);
      }
      _currentState = State::PlayingAnimation;
      _doReaction = false;
      return Status::Running;
    }
    case State::PlayingAnimation:
    {
      return Status::Running;
    }
    default:
    {
      PRINT_NAMED_ERROR("BehaviorReactToPoke.Update.UnknownState",
                        "Reached unknown state %d.", _currentState);
    }
  }
  
  return Status::Complete;
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
      if (_animTagToWaitFor == msg.idTag)
      {
        _currentState = State::Inactive;
      }
      break;
    }
    default:
    {
      break;
    }
  }
}

} // namespace Cozmo
} // namespace Anki