/**
 * File: behaviorLookAround.cpp
 *
 * Author: Lee
 * Created: 08/13/15
 *
 * Description: Behavior for looking around the environment for stuff to interact with.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorLookAround.h"
#include "anki/cozmo/basestation/cozmoActions.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/common/shared/radians.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
// Time to wait before looking around again in seconds
const float BehaviorLookAround::kLookAroundCooldownDuration = 20.0f;
const float BehaviorLookAround::kDegreesRotatePerSec = 22.5f;

BehaviorLookAround::BehaviorLookAround(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _currentState(State::INACTIVE)
  , _lastLookAroundTime(0.f)
{
  _name = "LookAround";
}

bool BehaviorLookAround::IsRunnable(float currentTime_sec) const
{
  switch (_currentState)
  {
    case State::INACTIVE:
    {
      if (_lastLookAroundTime + kLookAroundCooldownDuration < currentTime_sec)
      {
        return true;
      }
      break;
    }
    case State::START_LOOKING:
    case State::LOOKING_FOR_OBJECT:
    case State::EXAMINE_FOUND_OBJECT:
    {
      return true;
    }
    default:
    {
      PRINT_NAMED_ERROR("LookAround_Behavior.IsRunnable.UnknownState",
                        "Reached unknown state %d.", _currentState);
    }
  }
  return false;
}

Result BehaviorLookAround::Init()
{
  _currentState = State::START_LOOKING;
  return Result::RESULT_OK;
}

IBehavior::Status BehaviorLookAround::Update(float currentTime_sec)
{
  static Radians lastHeading = _robot.GetPose().GetRotationAngle<'Z'>();
  
  switch (_currentState)
  {
    case State::INACTIVE:
    {
      break;
    }
    case State::START_LOOKING:
    {
      _totalRotation = 0;
      _robot.TurnInPlaceAtSpeed(DEG_TO_RAD(kDegreesRotatePerSec), DEG_TO_RAD(1440));
      _currentState = State::LOOKING_FOR_OBJECT;
    }
    // NOTE INTENTIONAL FALLTHROUGH
    case State::LOOKING_FOR_OBJECT:
    {
      Radians newHeading = _robot.GetPose().GetRotationAngle<'Z'>();
      _totalRotation += (newHeading - lastHeading).getDegrees();
      lastHeading = newHeading;
      
      if (_totalRotation > 360.f)
      {
        ResetBehavior(currentTime_sec);
        break;
      }
      
      return Status::RUNNING;
    }
    case State::EXAMINE_FOUND_OBJECT:
    {
      return Status::RUNNING;
    }
    default:
    {
      PRINT_NAMED_ERROR("LookAround_Behavior.Update.UnknownState",
                        "Reached unknown state %d.", _currentState);
    }
  }
  
  return Status::COMPLETE;
}

Result BehaviorLookAround::Interrupt(float currentTime_sec)
{
  ResetBehavior(currentTime_sec);
  return Result::RESULT_OK;
}
  
void BehaviorLookAround::ResetBehavior(float currentTime_sec)
{
  _lastLookAroundTime = currentTime_sec;
  _currentState = State::INACTIVE;
  _robot.TurnInPlaceAtSpeed(DEG_TO_RAD(0), DEG_TO_RAD(1440));
}

bool BehaviorLookAround::GetRewardBid(Reward& reward)
{
  return true;
}


} // namespace Cozmo
} // namespace Anki