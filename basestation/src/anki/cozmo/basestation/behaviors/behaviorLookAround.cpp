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
  
using namespace ExternalInterface;

BehaviorLookAround::BehaviorLookAround(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _moveAreaCenter(robot.GetPose())
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
    
    // Register for RobotPutDown Event
    _eventHandles.push_back(interface->Subscribe(MessageEngineToGameTag::RobotPutDown,
                                                 std::bind(&BehaviorLookAround::HandleRobotPutDown, this, std::placeholders::_1)));
    
    
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
  switch (_currentState)
  {
    case State::Inactive:
    {
      // This is the last update before we stop running, so store off time
      _lastLookAroundTime = currentTime_sec;
      
      break; // Jump down and return Status::Complete
    }
    case State::StartLooking:
    {
      StartMoving();
      _currentState = State::LookingForObject;
    }
    // NOTE INTENTIONAL FALLTHROUGH
    case State::LookingForObject:
    {
      if (0 < _recentObjects.size())
      {
        _currentState = State::ExamineFoundObject;
      }
      return Status::Running;
    }
    case State::ExamineFoundObject:
    {
      _robot.GetActionList().Cancel();
      bool queuedFaceObjectAction = false;
      while (!_recentObjects.empty())
      {
        auto iter = _recentObjects.begin();
        ObjectID objID = *iter;
        IActionRunner* faceObjectAction = new FaceObjectAction(objID, Vision::Marker::ANY_CODE, DEG_TO_RAD(5), DEG_TO_RAD(1440), true);
        
        _robot.GetActionList().QueueActionAtEnd(0, faceObjectAction);
        queuedFaceObjectAction = true;
        
        ++_numObjectsToLookAt;
        _oldBoringObjects.insert(objID);
        _recentObjects.erase(iter);
      }
      
      // If we queued up some face object actions, add a move head action at the end to go back to normal
      if (queuedFaceObjectAction)
      {
        IActionRunner* moveHeadAction = new MoveHeadToAngleAction(0);
        _robot.GetActionList().QueueActionAtEnd(0, moveHeadAction);
      }
      _currentState = State::WaitToFinishExamining;
      
      return Status::Running;
    }
    case State::WaitToFinishExamining:
    {
      if (0 == _numObjectsToLookAt)
      {
        _currentState = State::StartLooking;
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
  
void BehaviorLookAround::StartMoving()
{
  IActionRunner* goToPoseAction = new DriveToPoseAction(GetDestinationPose(_currentDestination), false, false);
  _currentDriveActionID = goToPoseAction->GetTag();
  _robot.GetActionList().QueueActionAtEnd(0, goToPoseAction);
}
  
Pose3d BehaviorLookAround::GetDestinationPose(BehaviorLookAround::Destination destination)
{
  Pose3d destPose(_moveAreaCenter);
  switch (destination)
  {
    case Destination::North:
    case Destination::Center:
    {
      // Don't need to rotate here
      break;
    }
    case Destination::West:
    {
      destPose.RotateBy(Rotation3d(DEG_TO_RAD(90), Z_AXIS_3D()));
      break;
    }
    case Destination::South:
    {
      destPose.RotateBy(Rotation3d(DEG_TO_RAD(180), Z_AXIS_3D()));
      break;
    }
    case Destination::East:
    {
      destPose.RotateBy(Rotation3d(DEG_TO_RAD(-90), Z_AXIS_3D()));
      break;
    }
    case Destination::Count:
    default:
    {
      PRINT_NAMED_ERROR("LookAround_Behavior.GetDestinationPose.InvalidDestination",
                        "Reached invalid destination %d.", destination);
      break;
    }
  }
  
  if (Destination::Center != destination)
  {
    destPose.SetTranslation(destPose * Point3f(_safeRadius, 0, 0));
  }
  
  return destPose;
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
  _robot.GetActionList().Cancel();
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
  else if (msg.idTag == _currentDriveActionID)
  {
    if (ActionResult::SUCCESS == msg.result)
    {
      // If we're back to center, time to go inactive
      if (Destination::Center == _currentDestination)
      {
        _currentState = State::Inactive;
      }
      else
      {
        _currentState = State::StartLooking;
      }
      
      // If this was a successful drive action, move on to the next destination
      _currentDestination = GetNextDestination(_currentDestination);
    }
    else
    {
      // We didn't succeed for some reason - try again
      _currentState = State::StartLooking;
    }
  }
}
  
BehaviorLookAround::Destination BehaviorLookAround::GetNextDestination(BehaviorLookAround::Destination oldDest)
{
  switch (oldDest)
  {
    case Destination::North:
    {
      return Destination::West;
    }
    case Destination::West:
    {
      return Destination::South;
    }
    case Destination::South:
    {
      return Destination::East;
    }
    case Destination::East:
    {
      return Destination::Center;
    }
    case Destination::Center:
    {
      return Destination::North;
    }
    default:
    {
      PRINT_NAMED_ERROR("LookAround_Behavior.GetDestinationPose.InvalidDestination",
                        "Reached invalid destination %d.", oldDest);
      return Destination::North;
    }
  }
}
  
void BehaviorLookAround::HandleRobotPutDown(const AnkiEvent<MessageEngineToGame>& event)
{
  const RobotPutDown& msg = event.GetData().Get_RobotPutDown();
  if (_robot.GetID() == msg.robotID)
  {
    _moveAreaCenter = _robot.GetPose();
    _safeRadius = kDefaultSafeRadius;
  }
}

} // namespace Cozmo
} // namespace Anki