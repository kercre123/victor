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
const float BehaviorLookAround::kDegreesRotatePerSec = 25.0f;
  
using namespace ExternalInterface;

BehaviorLookAround::BehaviorLookAround(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _currentState(State::Inactive)
  , _lastLookAroundTime(0.f)
{
  _name = "LookAround";
  
  // We might not have an external interface pointer (e.g. Unit tests)
  if (robot.HasExternalInterface()) {
    // Register for the RobotObservedObject event
    IExternalInterface* interface = robot.GetExternalInterface();
    _eventHandles.push_back(interface->Subscribe(MessageEngineToGameTag::RobotObservedObject,
                                                 std::bind(&BehaviorLookAround::HandleObjectObserved, this, std::placeholders::_1)));
    
    // Register for RobotCompletedAction Event
    _eventHandles.push_back(interface->Subscribe(MessageEngineToGameTag::RobotCompletedAction,
                                                 std::bind(&BehaviorLookAround::HandleCompletedAction, this, std::placeholders::_1)));
  }
}

bool BehaviorLookAround::IsRunnable(float currentTime_sec) const
{
  switch (_currentState)
  {
    case State::Inactive:
    {
      if (_lastLookAroundTime + kLookAroundCooldownDuration < currentTime_sec)
      {
        return true;
      }
      break;
    }
    case State::StartLooking:
    case State::ContinueLooking:
    case State::LookingForObject:
    case State::ExamineFoundObject:
    case State::WaitToFinishExamining:
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
  _currentState = State::StartLooking;
  return Result::RESULT_OK;
}

IBehavior::Status BehaviorLookAround::Update(float currentTime_sec)
{
  static Radians lastHeading;
  
  switch (_currentState)
  {
    case State::Inactive:
    {
      break; // Jump down and return Status::Complete
    }
    case State::StartLooking:
    {
      _totalRotation = 0;
      lastHeading = _robot.GetPose().GetRotationAngle<'Z'>();
    }
    // NOTE INTENTIONAL FALLTHROUGH
    case State::ContinueLooking:
    {
      _robot.TurnInPlaceAtSpeed(DEG_TO_RAD(kDegreesRotatePerSec), DEG_TO_RAD(1440));
      _currentState = State::LookingForObject;
    }
    // NOTE INTENTIONAL FALLTHROUGH
    case State::LookingForObject:
    {
      Radians newHeading = _robot.GetPose().GetRotationAngle<'Z'>();
      _totalRotation += fabsf((newHeading - lastHeading).getDegrees());
      lastHeading = newHeading;
      
      if (_totalRotation > 360.f)
      {
        ResetBehavior(currentTime_sec);
        break; // Jump down and return Status::Complete
      }
      
      if (0 < _recentObjects.size())
      {
        _currentState = State::ExamineFoundObject;
      }
      return Status::Running;
    }
    case State::ExamineFoundObject:
    {
      _robot.StopAllMotors();
      while (!_recentObjects.empty())
      {
        auto iter = _recentObjects.begin();
        ObjectID objID = *iter;
        IActionRunner* faceObjectAction = new FaceObjectAction(objID, DEG_TO_RAD(5), DEG_TO_RAD(1440));
        IActionRunner* moveHeadAction = new MoveHeadToAngleAction(0);
        
        _robot.GetActionList().QueueActionAtEnd(0, faceObjectAction);
        _robot.GetActionList().QueueActionAtEnd(0, moveHeadAction);
        
        ++_numObjectsToLookAt;
        _oldBoringObjects.insert(objID);
        _recentObjects.erase(iter);
      }
      _currentState = State::WaitToFinishExamining;
      
      return Status::Running;
    }
    case State::WaitToFinishExamining:
    {
      if (0 == _numObjectsToLookAt)
      {
        _currentState = State::ContinueLooking;
      }
      return Status::Running;
    }
    default:
    {
      PRINT_NAMED_ERROR("LookAround_Behavior.Update.UnknownState",
                        "Reached unknown state %d.", _currentState);
    }
  }
  
  return Status::Complete;
}

Result BehaviorLookAround::Interrupt(float currentTime_sec)
{
  ResetBehavior(currentTime_sec);
  return Result::RESULT_OK;
}
  
void BehaviorLookAround::ResetBehavior(float currentTime_sec)
{
  _lastLookAroundTime = currentTime_sec;
  _currentState = State::Inactive;
  _robot.StopAllMotors();
  _recentObjects.clear();
  _oldBoringObjects.clear();
  _numObjectsToLookAt = 0;
}

bool BehaviorLookAround::GetRewardBid(Reward& reward)
{
  return true;
}
  
void BehaviorLookAround::HandleObjectObserved(const AnkiEvent<MessageEngineToGame>& event)
{
  // Ignore messages while not running
  if(!IsRunning())
  {
    return;
  }
  
  const RobotObservedObject& msg = event.GetData().Get_RobotObservedObject();
  
  // We'll get continuous updates about objects in view, so only care about new ones whose markers we can see
  if (0 == _oldBoringObjects.count(msg.objectID) && msg.markersVisible)
  {
    _recentObjects.insert(msg.objectID);
  }
}
  
void BehaviorLookAround::HandleCompletedAction(const AnkiEvent<MessageEngineToGame>& event)
{
  if(!IsRunning())
  {
    // Ignore messages while not running
    return;
  }
  
  const RobotCompletedAction& msg = event.GetData().Get_RobotCompletedAction();
  if (RobotActionType::FACE_OBJECT == msg.actionType)
  {
    if (0 == _numObjectsToLookAt)
    {
      PRINT_NAMED_WARNING("BehaviorLookAround.HandleCompletedAction", "Getting unexpected FaceObjectAction completion messages");
    }
    else
    {
      --_numObjectsToLookAt;
    }
  }
}

} // namespace Cozmo
} // namespace Anki