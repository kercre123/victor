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
#include "anki/cozmo/basestation/trackingActions.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"

#define DEBUG_ANIMATION_LOCKING 0

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

// TODO:(bn) move this somewhere central
namespace AnimTrackFlagHelpers
{

std::string AnimTrackFlagsToString(uint8_t tracks)
{
  std::stringstream ss;
  for (int i=0; i < (int)AnimConstants::NUM_TRACKS; i++)
  {
    uint8_t currTrack = (1 << i);
    if( tracks & currTrack ) {
      ss << EnumToString( static_cast<AnimTrackFlag>(currTrack) ) << ' ';
    }
  }

  return ss.str();
}
}


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
    // Note that this will temporarily take over the head but because it's not an action,
    // any tracking that's using the head should just re-take control on next
    // observation.
    MoveHeadToAngle(msg.angle_rad, msg.max_speed_rad_per_sec, msg.accel_rad_per_sec2, msg.duration_sec);
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
  
bool MovementComponent::IsAnimTrackLocked(AnimTrackFlag track) const
{
  return _animTrackLockCount[GetFlagIndex((uint8_t)track)] > 0;
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
#if DEBUG_ANIMATION_LOCKING
  PRINT_NAMED_INFO("MovementComponent.LockAnimTracks", "locked: (0x%x) %s, result:",
                   tracks,
                   AnimTrackFlagHelpers::AnimTrackFlagsToString(tracks).c_str());
  PrintAnimationLockState();
#endif
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
#if DEBUG_ANIMATION_LOCKING
  PRINT_NAMED_INFO("MovementComponent.LockAnimTracks", "unlocked: (0x%x) %s, result:",
                   tracks,
                   AnimTrackFlagHelpers::AnimTrackFlagsToString(tracks).c_str());
  PrintAnimationLockState();
#endif
}

void MovementComponent::PrintAnimationLockState() const
{
  std::stringstream ss;
  for( int trackNum = 0; trackNum < (int)AnimConstants::NUM_TRACKS; ++trackNum ) {
    if( _animTrackLockCount[trackNum] > 0 ) {
      uint8_t trackEnumVal = 1 << trackNum;
      ss << AnimTrackFlagHelpers::AnimTrackFlagsToString(trackEnumVal) << ":" << _animTrackLockCount[trackNum] << ' ';
    }
  }

  PRINT_NAMED_DEBUG("MovementComponent.AnimationLocks", "%s", ss.str().c_str());
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
