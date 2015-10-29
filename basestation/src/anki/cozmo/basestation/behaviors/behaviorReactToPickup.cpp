/**
 * File: behaviorReactToPickup.cpp
 *
 * Author: Lee
 * Created: 08/26/15
 *
 * Description: Behavior for immediately responding being picked up.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorReactToPickup.h"
#include "anki/cozmo/basestation/cozmoActions.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;
  
static std::vector<std::string> _animReactions = {
  "Demo_Face_Interaction_ShockedScared_A",
};

BehaviorReactToPickup::BehaviorReactToPickup(Robot& robot, const Json::Value& config)
: IReactionaryBehavior(robot, config)
{
  _name = "ReactToPickup";
  
  // These are the tags that should trigger this behavior to be switched to immediately
  auto triggerTags = { MessageEngineToGameTag::RobotPickedUp };
  _engineToGameTags = triggerTags;
  
  // We're going to subscribe to our trigger events, plus others
  std::vector<MessageEngineToGameTag> subscribedEvents = triggerTags;
  subscribedEvents.insert(subscribedEvents.end(), {
    MessageEngineToGameTag::RobotPutDown,
    MessageEngineToGameTag::RobotCompletedAction,
  });
  
  SubscribeToTags(std::move(subscribedEvents));
}

bool BehaviorReactToPickup::IsRunnable(const Robot& robot, double currentTime_sec) const
{
  switch (_currentState)
  {
    case State::Inactive:
    case State::IsPickedUp:
    case State::PlayingAnimation:
    {
      return true;
    }
    default:
    {
      PRINT_NAMED_ERROR("BehaviorReactToPickup.IsRunnable.UnknownState",
                        "Reached unknown state %d.", _currentState);
    }
  }
  return false;
}

Result BehaviorReactToPickup::InitInternal(Robot& robot, double currentTime_sec)
{
  return Result::RESULT_OK;
}

IBehavior::Status BehaviorReactToPickup::UpdateInternal(Robot& robot, double currentTime_sec)
{
  switch (_currentState)
  {
    case State::Inactive:
    {
      if (_isInAir)
      {
        _currentState = State::IsPickedUp;
        return Status::Running;
      }
      break; // Jump down and return Status::Complete
    }
    case State::IsPickedUp:
    {
      static u32 animIndex = 0;
      // For now we simply rotate through the animations we want to play when picked up
      if (!_animReactions.empty())
      {
        IActionRunner* newAction = new PlayAnimationAction(_animReactions[animIndex]);
        _animTagToWaitFor = newAction->GetTag();
        robot.GetActionList().QueueActionNow(0, newAction);
        animIndex = ++animIndex % _animReactions.size();
      }
      _waitingForAnimComplete = true;
      _currentState = State::PlayingAnimation;
      return Status::Running;
    }
    case State::PlayingAnimation:
    {
      if (!_waitingForAnimComplete)
      {
        // If our animation is done and we're not in the air, we're done
        if (!_isInAir)
        {
          _currentState = State::Inactive;
          break; // Jump down to Status::Complete
        }
        // Otherwise set our state to start the animation again
        _currentState = State::IsPickedUp;
      }
      return Status::Running;
    }
    default:
    {
      PRINT_NAMED_ERROR("BehaviorReactToPickup.Update.UnknownState",
                        "Reached unknown state %d.", _currentState);
    }
  }
  
  return Status::Complete;
}

Result BehaviorReactToPickup::InterruptInternal(Robot& robot, double currentTime_sec)
{
  // We don't want to be interrupted unless we're done reacting
  if (State::Inactive != _currentState)
  {
    return Result::RESULT_FAIL;
  }
  return Result::RESULT_OK;
}

void BehaviorReactToPickup::AlwaysHandle(const EngineToGameEvent& event,
                                         const Robot& robot)
{
  // We want to get these messages, even when not running
  switch (event.GetData().GetTag())
  {
    case MessageEngineToGameTag::RobotPickedUp:
    {
      _isInAir = true;
      break;
    }
    case MessageEngineToGameTag::RobotPutDown:
    {
      _isInAir = false;
      break;
    }
    case MessageEngineToGameTag::RobotCompletedAction:
    {
      const RobotCompletedAction& msg = event.GetData().Get_RobotCompletedAction();
      if (_animTagToWaitFor == msg.idTag)
      {
        _waitingForAnimComplete = false;
      }
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("BehaviorReactToPickup.HandleMovedEvent.UnknownEvent",
                        "Reached unknown state %d.", _currentState);
    }
  }
}

} // namespace Cozmo
} // namespace Anki