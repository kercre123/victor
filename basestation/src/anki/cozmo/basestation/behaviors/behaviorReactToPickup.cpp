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

BehaviorReactToPickup::BehaviorReactToPickup(Robot& robot, const Json::Value& config)
  : IReactionaryBehavior(robot, config)
  , _currentState(State::Inactive)
{
  _name = "ReactToPickup";
  
  // These are the tags that should trigger this behavior to be switched to immediately
  auto triggerTags = { MessageEngineToGameTag::ActiveObjectMoved };
  _engineToGameTags = triggerTags;
  
  // We're going to subscribe to our trigger events, plus others
  std::vector<MessageEngineToGameTag> subscribedEvents = triggerTags;
  subscribedEvents.insert(subscribedEvents.end(), {
    MessageEngineToGameTag::ActiveObjectStoppedMoving,
    MessageEngineToGameTag::RobotCompletedAction,
  });
  
  // We might not have an external interface pointer (e.g. Unit tests)
  if (robot.HasExternalInterface()) {
    // Register for EngineToGameEvents
    for (auto tag : subscribedEvents)
    {
      _eventHandles.push_back(robot.GetExternalInterface()->Subscribe(tag, std::bind(&BehaviorReactToPickup::HandleMovedEvent, this, std::placeholders::_1)));
    }
  }
}

bool BehaviorReactToPickup::IsRunnable(float currentTime_sec) const
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

Result BehaviorReactToPickup::Init()
{
  return Result::RESULT_OK;
}

IBehavior::Status BehaviorReactToPickup::Update(float currentTime_sec)
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
      _robot.GetActionList().QueueActionNow(0, new PlayAnimationAction("majorWin"));
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

Result BehaviorReactToPickup::Interrupt(float currentTime_sec)
{
  // We don't want to be interrupted unless we're done reacting
  if (State::Inactive != _currentState)
  {
    return Result::RESULT_FAIL;
  }
  return Result::RESULT_OK;
}

bool BehaviorReactToPickup::GetRewardBid(Reward& reward)
{
  return true;
}

void BehaviorReactToPickup::HandleMovedEvent(const AnkiEvent<MessageEngineToGame>& event)
{
  // We want to get these messages, even when not running
  switch (event.GetData().GetTag())
  {
    case MessageEngineToGameTag::ActiveObjectMoved:
    {
      _isInAir = true;
      break;
    }
    case MessageEngineToGameTag::ActiveObjectStoppedMoving:
    {
      _isInAir = false;
      break;
    }
    case MessageEngineToGameTag::RobotCompletedAction:
    {
      _waitingForAnimComplete = false;
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