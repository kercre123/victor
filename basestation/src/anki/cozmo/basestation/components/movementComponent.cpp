/**
 * File: movementComponent.cpp
 *
 * Author: Lee Crippen
 * Created: 10/21/2015
 *
 * Description: Robot component to handle logic and messages associated with the robot moving.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/cozmo/basestation/components/movementComponent.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

MovementComponent::MovementComponent(Robot& robot)
  : _robot(robot)
{
  if (_robot.HasExternalInterface())
  {
    InitEventHandlers(*(_robot.GetExternalInterface()));
  }
}
  
void MovementComponent::InitEventHandlers(IExternalInterface& interface)
{
  // Lambda for handling DriveWheels
  _eventHandles.push_back(interface.Subscribe(MessageGameToEngineTag::DriveWheels,
  [this] (const AnkiEvent<MessageGameToEngine>& event)
  {
    const struct DriveWheels& msg = event.GetData().Get_DriveWheels();
    if(AreWheelsLocked()) {
      PRINT_NAMED_INFO("CozmoGameImpl.Process_DriveWheels.WheelsLocked",
                       "Ignoring ExternalInterface::DriveWheels while wheels are locked.\n");
    } else {
      DriveWheels(msg.lwheel_speed_mmps, msg.rwheel_speed_mmps);
    }
  }));
  
  // Lambda for handling TurnInPlaceAtSpeed
  _eventHandles.push_back(interface.Subscribe(MessageGameToEngineTag::TurnInPlaceAtSpeed,
  [this] (const AnkiEvent<MessageGameToEngine>& event)
  {
    const struct TurnInPlaceAtSpeed& msg = event.GetData().Get_TurnInPlaceAtSpeed();
    TurnInPlaceAtSpeed(msg.speed_rad_per_sec, msg.accel_rad_per_sec2);
  }));
  
  // Lambda for handling MoveHead
  _eventHandles.push_back(interface.Subscribe(MessageGameToEngineTag::MoveHead,
  [this] (const AnkiEvent<MessageGameToEngine>& event)
  {
    const struct MoveHead& msg = event.GetData().Get_MoveHead();
    if(IsHeadLocked()) {
      PRINT_NAMED_INFO("CozmoGameImpl.Process_MoveHead.HeadLocked",
                       "Ignoring ExternalInterface::MoveHead while head is locked.");
    } else {
      MoveHead(msg.speed_rad_per_sec);
    }
  }));
  
  // Lambda for handling MoveLift
  _eventHandles.push_back(interface.Subscribe(MessageGameToEngineTag::MoveLift,
  [this] (const AnkiEvent<MessageGameToEngine>& event)
  {
    const struct MoveLift& msg = event.GetData().Get_MoveLift();
    if(IsLiftLocked()) {
      PRINT_NAMED_INFO("CozmoGameImpl.Process_MoveLift.LiftLocked",
                       "Ignoring ExternalInterface::MoveLift while lift is locked.");
    } else {
      MoveLift(msg.speed_rad_per_sec);
    }
  }));
  
  // Lambda for handling SetHeadAngle
  _eventHandles.push_back(interface.Subscribe(MessageGameToEngineTag::SetHeadAngle,
  [this] (const AnkiEvent<MessageGameToEngine>& event)
  {
    const struct SetHeadAngle& msg = event.GetData().Get_SetHeadAngle();
    if(IsHeadLocked()) {
      PRINT_NAMED_INFO("CozmoGameImpl.Process_SetHeadAngle.HeadLocked",
                       "Ignoring ExternalInterface::SetHeadAngle while head is locked.\n");
    } else {
      DisableTrackToObject();
      MoveHeadToAngle(msg.angle_rad, msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2, msg.duration_sec);
    }
  }));
  
  // Lambda for handling TrackToObject
  _eventHandles.push_back(interface.Subscribe(MessageGameToEngineTag::TrackToObject,
  [this] (const AnkiEvent<MessageGameToEngine>& event)
  {
    const struct TrackToObject& msg = event.GetData().Get_TrackToObject();
    if(IsHeadLocked()) {
      PRINT_NAMED_INFO("CozmoGameImpl.Process_TrackHeadToObject.HeadLocked",
                       "Ignoring ExternalInterface::TrackHeadToObject while head is locked.\n");
    } else {
      if(msg.objectID == u32_MAX) {
        DisableTrackToObject();
      } else {
        EnableTrackToObject(msg.objectID, msg.headOnly);
      }
    }
  }));
  
  // Lambda for handling StopAllMotors
  _eventHandles.push_back(interface.Subscribe(MessageGameToEngineTag::StopAllMotors,
  [this] (const AnkiEvent<MessageGameToEngine>& event)
  {
    StopAllMotors();
  }));
}
  
  
// =========== Motor commands ============

// Sends message to move lift at specified speed
Result MovementComponent::MoveLift(const f32 speed_rad_per_sec)
{
  return _robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::MoveLift(speed_rad_per_sec)));
}

// Sends message to move head at specified speed
Result MovementComponent::MoveHead(const f32 speed_rad_per_sec)
{
  return _robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::MoveHead(speed_rad_per_sec)));
}

// Sends a message to the robot to move the lift to the specified height
Result MovementComponent::MoveLiftToHeight(const f32 height_mm,
                                           const f32 max_speed_rad_per_sec,
                                           const f32 accel_rad_per_sec2,
                                           const f32 duration_sec)
{
  return _robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::SetLiftHeight(height_mm,
                                                                                        max_speed_rad_per_sec,
                                                                                        accel_rad_per_sec2,
                                                                                        duration_sec)));
}

// Sends a message to the robot to move the head to the specified angle
Result MovementComponent::MoveHeadToAngle(const f32 angle_rad,
                                          const f32 max_speed_rad_per_sec,
                                          const f32 accel_rad_per_sec2,
                                          const f32 duration_sec)
{
  return _robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::SetHeadAngle(angle_rad,
                                                                                       max_speed_rad_per_sec,
                                                                                       accel_rad_per_sec2,
                                                                                       duration_sec)));
}

Result MovementComponent::TurnInPlaceAtSpeed(const f32 speed_rad_per_sec,
                                             const f32 accel_rad_per_sec2)
{
  return _robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::TurnInPlaceAtSpeed(speed_rad_per_sec,
                                                                                             accel_rad_per_sec2)));
}

Result MovementComponent::DriveWheels(const f32 lwheel_speed_mmps,
                                      const f32 rwheel_speed_mmps)
{
  return _robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::DriveWheels(lwheel_speed_mmps,
                                                                                      rwheel_speed_mmps)));
}

Result MovementComponent::StopAllMotors()
{
  return _robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::StopAllMotors()));
}
  
Result MovementComponent::EnableTrackToObject(const u32 objectID, bool headOnly)
{
  _trackToObjectID = objectID;
  
  if(_robot.GetBlockWorld().GetObjectByID(_trackToObjectID) != nullptr) {
    _trackWithHeadOnly = headOnly;
    _trackToFaceID = Vision::TrackedFace::UnknownFace;
    
    // Store whether head/wheels were locked before tracking so we can
    // return them to this state when we disable tracking
    _headLockedBeforeTracking = IsHeadLocked();
    _wheelsLockedBeforeTracking = AreWheelsLocked();
    
    LockHead(true);
    if(!headOnly) {
      LockWheels(true);
    }
    
    return RESULT_OK;
  } else {
    PRINT_NAMED_ERROR("Robot.EnableTrackToObject.UnknownObject",
                      "Cannot track to object ID=%d, which does not exist.\n",
                      objectID);
    _trackToObjectID.UnSet();
    return RESULT_FAIL;
  }
}

Result MovementComponent::EnableTrackToFace(Vision::TrackedFace::ID_t faceID, bool headOnly)
{
  _trackToFaceID = faceID;
  if(_robot.GetFaceWorld().GetFace(_trackToFaceID) != nullptr) {
    _trackWithHeadOnly = headOnly;
    _trackToObjectID.UnSet();
    
    // Store whether head/wheels were locked before tracking so we can
    // return them to this state when we disable tracking        // Store whether head/wheels were locked before tracking so we can
    // return them to this state when we disable tracking
    _headLockedBeforeTracking = IsHeadLocked();
    _wheelsLockedBeforeTracking = AreWheelsLocked();
    
    LockHead(true);
    if(!headOnly) {
      LockWheels(true);
    }
    
    return RESULT_OK;
  } else {
    PRINT_NAMED_ERROR("Robot.EnableTrackToFace.UnknownFace",
                      "Cannot track to face ID=%lld, which does not exist.",
                      faceID);
    _trackToFaceID = Vision::TrackedFace::UnknownFace;
    return RESULT_FAIL;
  }
}

Result MovementComponent::DisableTrackToObject()
{
  if(_trackToObjectID.IsSet()) {
    _trackToObjectID.UnSet();
    // Restore lock state to whatever it was when we enabled tracking
    LockHead(_headLockedBeforeTracking);
    LockWheels(_wheelsLockedBeforeTracking);
  }
  return RESULT_OK;
}

Result MovementComponent::DisableTrackToFace()
{
  if(_trackToFaceID != Vision::TrackedFace::UnknownFace) {
    _trackToFaceID = Vision::TrackedFace::UnknownFace;
    // Restore lock state to whatever it was when we enabled tracking
    LockHead(_headLockedBeforeTracking);
    LockWheels(_wheelsLockedBeforeTracking);
  }
  return RESULT_OK;
}

} // namespace Cozmo
} // namespace Anki