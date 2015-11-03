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
#include "anki/cozmo/basestation/ankiEventUtil.h"
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
  auto helper = AnkiEventUtil<MovementComponent, decltype(_eventHandles)>(interface, *this, _eventHandles);
  
  helper.SubscribeInternal<MessageGameToEngineTag::DriveWheels>();
  helper.SubscribeInternal<MessageGameToEngineTag::TurnInPlaceAtSpeed>();
  helper.SubscribeInternal<MessageGameToEngineTag::MoveHead>();
  helper.SubscribeInternal<MessageGameToEngineTag::MoveLift>();
  helper.SubscribeInternal<MessageGameToEngineTag::SetHeadAngle>();
  helper.SubscribeInternal<MessageGameToEngineTag::TrackToObject>();
  helper.SubscribeInternal<MessageGameToEngineTag::StopAllMotors>();
}
  
template<>
void MovementComponent::HandleMessage(const ExternalInterface::DriveWheels& msg)
{
  if(AreWheelsLocked()) {
    PRINT_NAMED_INFO("MovementComponent.EventHandler.DriveWheels.WheelsLocked",
                     "Ignoring ExternalInterface::DriveWheels while wheels are locked.");
  } else {
    _robot.SendRobotMessage<RobotInterface::DriveWheels>(msg.lwheel_speed_mmps, msg.rwheel_speed_mmps);
  }
}

template<>
void MovementComponent::HandleMessage(const ExternalInterface::TurnInPlaceAtSpeed& msg)
{
  _robot.SendRobotMessage<RobotInterface::TurnInPlaceAtSpeed>(msg.speed_rad_per_sec, msg.accel_rad_per_sec2);
}

template<>
void MovementComponent::HandleMessage(const ExternalInterface::MoveHead& msg)
{
  if(IsHeadLocked()) {
    PRINT_NAMED_INFO("MovementComponent.EventHandler.MoveHead.HeadLocked",
                     "Ignoring ExternalInterface::MoveHead while head is locked.");
  } else {
    _robot.SendRobotMessage<RobotInterface::MoveHead>(msg.speed_rad_per_sec);
  }
}

template<>
void MovementComponent::HandleMessage(const ExternalInterface::MoveLift& msg)
{
  if(IsLiftLocked()) {
    PRINT_NAMED_INFO("MovementComponent.EventHandler.MoveLift.LiftLocked",
                     "Ignoring ExternalInterface::MoveLift while lift is locked.");
  } else {
    _robot.SendRobotMessage<RobotInterface::MoveLift>(msg.speed_rad_per_sec);
  }
}

template<>
void MovementComponent::HandleMessage(const ExternalInterface::SetHeadAngle& msg)
{
  if(IsHeadLocked()) {
    PRINT_NAMED_INFO("MovementComponent.EventHandler.SetHeadAngle.HeadLocked",
                     "Ignoring ExternalInterface::SetHeadAngle while head is locked.");
  } else {
    DisableTrackToObject();
    MoveHeadToAngle(msg.angle_rad, msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2, msg.duration_sec);
  }
}

template<>
void MovementComponent::HandleMessage(const ExternalInterface::TrackToObject& msg)
{
  if(IsHeadLocked()) {
    PRINT_NAMED_INFO("MovementComponent.EventHandler.TrackHeadToObject.HeadLocked",
                     "Ignoring ExternalInterface::TrackHeadToObject while head is locked.");
  } else {
    if(msg.objectID == u32_MAX) {
      DisableTrackToObject();
    } else {
      EnableTrackToObject(msg.objectID, msg.headOnly);
    }
  }
}

template<>
void MovementComponent::HandleMessage(const ExternalInterface::StopAllMotors& msg)
{
  StopAllMotors();
}
  
// =========== Motor commands ============

// Sends a message to the robot to move the lift to the specified height
Result MovementComponent::MoveLiftToHeight(const f32 height_mm,
                                           const f32 max_speed_rad_per_sec,
                                           const f32 accel_rad_per_sec2,
                                           const f32 duration_sec)
{
  return _robot.SendRobotMessage<RobotInterface::SetLiftHeight>(height_mm,
                                                                max_speed_rad_per_sec,
                                                                accel_rad_per_sec2,
                                                                duration_sec);
}

// Sends a message to the robot to move the head to the specified angle
Result MovementComponent::MoveHeadToAngle(const f32 angle_rad,
                                          const f32 max_speed_rad_per_sec,
                                          const f32 accel_rad_per_sec2,
                                          const f32 duration_sec)
{
  return _robot.SendRobotMessage<RobotInterface::SetHeadAngle>(angle_rad,
                                                               max_speed_rad_per_sec,
                                                               accel_rad_per_sec2,
                                                               duration_sec);
}

Result MovementComponent::StopAllMotors()
{
  return _robot.SendRobotMessage<RobotInterface::StopAllMotors>();
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
    PRINT_NAMED_ERROR("MovementComponent.EnableTrackToObject.UnknownObject",
                      "Cannot track to object ID=%d, which does not exist.",
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
    PRINT_NAMED_ERROR("MovementComponent.EnableTrackToFace.UnknownFace",
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