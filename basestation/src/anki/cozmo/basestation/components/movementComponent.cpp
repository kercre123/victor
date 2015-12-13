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
  , _animTrackLockCount((int)AnimConstants::NUM_TRACKS)
  , _ignoreTrackMovementCount((int)AnimConstants::NUM_TRACKS)
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
  helper.SubscribeInternal<MessageGameToEngineTag::TrackToFace>();
  helper.SubscribeInternal<MessageGameToEngineTag::StopAllMotors>();
}
  
template<>
void MovementComponent::HandleMessage(const ExternalInterface::DriveWheels& msg)
{
  if(IsMovementTrackIgnored(AnimTrackFlag::BODY_TRACK)) {
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
  if(IsMovementTrackIgnored(AnimTrackFlag::HEAD_TRACK)) {
    PRINT_NAMED_INFO("MovementComponent.EventHandler.MoveHead.HeadLocked",
                     "Ignoring ExternalInterface::MoveHead while head is locked.");
  } else {
    _robot.SendRobotMessage<RobotInterface::MoveHead>(msg.speed_rad_per_sec);
  }
}

template<>
void MovementComponent::HandleMessage(const ExternalInterface::MoveLift& msg)
{
  if(IsMovementTrackIgnored(AnimTrackFlag::LIFT_TRACK)) {
    PRINT_NAMED_INFO("MovementComponent.EventHandler.MoveLift.LiftLocked",
                     "Ignoring ExternalInterface::MoveLift while lift is locked.");
  } else {
    _robot.SendRobotMessage<RobotInterface::MoveLift>(msg.speed_rad_per_sec);
  }
}

template<>
void MovementComponent::HandleMessage(const ExternalInterface::SetHeadAngle& msg)
{
  if(IsMovementTrackIgnored(AnimTrackFlag::HEAD_TRACK)) {
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
  if(IsMovementTrackIgnored(AnimTrackFlag::HEAD_TRACK)) {
    PRINT_NAMED_INFO("MovementComponent.EventHandler.TrackHeadToObject.HeadLocked",
                     "Ignoring ExternalInterface::TrackToObject while head is locked.");
  } else if (IsMovementTrackIgnored(AnimTrackFlag::BODY_TRACK) && !msg.headOnly) {
    PRINT_NAMED_INFO("MovementComponent.EventHandler.TrackHeadToObject.WheelsLocked",
                     "Ignoring ExternalInterface::TrackToObject while wheels are locked and headOnly == false.");
  } else {
    if(msg.objectID == u32_MAX) {
      DisableTrackToObject();
    } else {
      EnableTrackToObject(msg.objectID, msg.headOnly);
    }
  }
}

template<>
void MovementComponent::HandleMessage(const ExternalInterface::TrackToFace& msg)
{
  if(IsMovementTrackIgnored(AnimTrackFlag::HEAD_TRACK)) {
    PRINT_NAMED_INFO("MovementComponent.EventHandler.TrackToFace.HeadLocked",
                     "Ignoring ExternalInterface::TrackToFace while head is locked.");
  } else if (IsMovementTrackIgnored(AnimTrackFlag::BODY_TRACK) && !msg.headOnly) {
    PRINT_NAMED_INFO("MovementComponent.EventHandler.TrackHeadToFace.WheelsLocked",
                     "Ignoring ExternalInterface::TrackToFace while wheels are locked and headOnly == false.");
  } else {
    if(msg.faceID == u32_MAX) {
      DisableTrackToFace();
    } else {
      EnableTrackToFace(msg.faceID, msg.headOnly);
    }
  }
}
  
template<>
void MovementComponent::HandleMessage(const ExternalInterface::StopAllMotors& msg)
{
  StopAllMotors();
}
  
// =========== Motor commands ============


Result MovementComponent::EnableLiftPower(bool enable)
{
  return _robot.SendRobotMessage<RobotInterface::EnableLiftPower>(enable);
}

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

    // TODO:(bn) this doesn't seem to work (wheels aren't locked), but not sure if it should...
    uint8_t tracksToLock = (uint8_t)AnimTrackFlag::HEAD_TRACK;
    if(!headOnly) {
      tracksToLock |= (uint8_t)AnimTrackFlag::BODY_TRACK;
    }
    LockAnimTracks(tracksToLock);
    
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
    
    uint8_t tracksToLock = (uint8_t)AnimTrackFlag::HEAD_TRACK;
    if(!headOnly) {
      tracksToLock |= (uint8_t)AnimTrackFlag::BODY_TRACK;
    }
    LockAnimTracks(tracksToLock);
    
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
    
    uint8_t tracksToUnlock = (uint8_t)AnimTrackFlag::HEAD_TRACK;
    if(!_trackWithHeadOnly) {
      tracksToUnlock |= (uint8_t)AnimTrackFlag::BODY_TRACK;
    }
    UnlockAnimTracks(tracksToUnlock);
  }
  return RESULT_OK;
}

Result MovementComponent::DisableTrackToFace()
{
  if(_trackToFaceID != Vision::TrackedFace::UnknownFace) {
    _trackToFaceID = Vision::TrackedFace::UnknownFace;
    
    uint8_t tracksToUnlock = (uint8_t)AnimTrackFlag::HEAD_TRACK;
    if(!_trackWithHeadOnly) {
      tracksToUnlock |= (uint8_t)AnimTrackFlag::BODY_TRACK;
    }
    UnlockAnimTracks(tracksToUnlock);
  }
  return RESULT_OK;
}
  
int MovementComponent::GetFlagIndex(uint8_t flag) const
{
  int i = 0;
  while(flag > 1)
  {
    i++;
    flag = flag >> 1;
  }
  return i;
}

void MovementComponent::LockAnimTracks(uint8_t tracks)
{
  for (int i=0; i < (int)AnimConstants::NUM_TRACKS; i++)
  {
    uint8_t curTrack = (1 << i);
    if ((tracks & curTrack) == curTrack)
    {
      ++_animTrackLockCount[i];
      
      // If we just went from not locked to locked, inform the robot
      if (_animTrackLockCount[i] == 1)
      {
        _robot.SendMessage(RobotInterface::EngineToRobot(AnimKeyFrame::DisableAnimTracks(curTrack)));
      }
    }
  }
}

void MovementComponent::UnlockAnimTracks(uint8_t tracks)
{
  for (int i=0; i < (int)AnimConstants::NUM_TRACKS; i++)
  {
    uint8_t curTrack = (1 << i);
    if ((tracks & curTrack) == curTrack)
    {
      --_animTrackLockCount[i];
      
      // If we just went from locked to not locked, inform the robot
      if (_animTrackLockCount[i] == 0)
      {
        _robot.SendMessage(RobotInterface::EngineToRobot(AnimKeyFrame::EnableAnimTracks(curTrack)));
      }
      ASSERT_NAMED(_animTrackLockCount[i] >= 0, "Should have a matching number of anim track lock and unlocks!");
    }
  }
}
  
void MovementComponent::IgnoreTrackMovement(uint8_t tracks)
{
  for (int i=0; i < (int)AnimConstants::NUM_TRACKS; i++)
  {
    uint8_t curTrack = (1 << i);
    if ((tracks & curTrack) == curTrack)
    {
      ++_ignoreTrackMovementCount[i];
    }
  }
}
  
void MovementComponent::UnignoreTrackMovement(uint8_t tracks)
{
  for (int i=0; i < (int)AnimConstants::NUM_TRACKS; i++)
  {
    uint8_t curTrack = (1 << i);
    if ((tracks & curTrack) == curTrack)
    {
      --_ignoreTrackMovementCount[i];
      ASSERT_NAMED(_ignoreTrackMovementCount[i] >= 0, "Should have a matching number of ignore/unignore track movement!");
    }
  }
}
  
bool MovementComponent::IsMovementTrackIgnored(AnimTrackFlag track) const
{
  return _ignoreTrackMovementCount[GetFlagIndex((uint8_t)track)] > 0;
}

} // namespace Cozmo
} // namespace Anki
