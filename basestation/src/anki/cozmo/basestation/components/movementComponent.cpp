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
#include "anki/cozmo/basestation/components/animTrackHelpers.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/actions/trackingActions.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "util/console/consoleInterface.h"

#define DEBUG_ANIMATION_LOCKING 0

namespace Anki {
namespace Cozmo {
  
  CONSOLE_VAR(bool, kDebugTrackLocking, "Robot", false);
  
using namespace ExternalInterface;


MovementComponent::MovementComponent(Robot& robot)
  : _robot(robot)
{
  if (_robot.HasExternalInterface())
  {
    InitEventHandlers(*(_robot.GetExternalInterface()));
  }
  _trackLockCount.fill(0);
}
  
void MovementComponent::InitEventHandlers(IExternalInterface& interface)
{
  auto helper = MakeAnkiEventUtil(interface, *this, _eventHandles);
  
  helper.SubscribeGameToEngine<MessageGameToEngineTag::DriveWheels>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::TurnInPlaceAtSpeed>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::MoveHead>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::MoveLift>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::StopAllMotors>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::DriveArc>();
}
  
void MovementComponent::Update(const Cozmo::RobotState& robotState)
{
  _isMoving     =  static_cast<bool>(robotState.status & (uint16_t)RobotStatusFlag::IS_MOVING);
  _isHeadMoving = !static_cast<bool>(robotState.status & (uint16_t)RobotStatusFlag::HEAD_IN_POS);
  _isLiftMoving = !static_cast<bool>(robotState.status & (uint16_t)RobotStatusFlag::LIFT_IN_POS);
  _areWheelsMoving = static_cast<bool>(robotState.status & (uint16_t)RobotStatusFlag::ARE_WHEELS_MOVING);
  
  for(auto layerIter = _faceLayerTagsToRemoveOnHeadMovement.begin();
      layerIter != _faceLayerTagsToRemoveOnHeadMovement.end(); )
  {
    FaceLayerToRemove & layer = layerIter->second;
    if(_isHeadMoving && false == layer.headWasMoving) {
      // Wait for transition from stopped to moving again
      _robot.GetAnimationStreamer().RemovePersistentFaceLayer(layerIter->first, layer.duration_ms);
      layerIter = _faceLayerTagsToRemoveOnHeadMovement.erase(layerIter);
    } else {
      layer.headWasMoving = _isHeadMoving;
      ++layerIter;
    }
  }
  if( kDebugTrackLocking )
  {
    // Flip logic from enabled to locked here, since robot stores bits as enabled and 1 means locked here.
    u8 robotLockedTracks = ~_robot.GetEnabledAnimationTracks();
    const bool isRobotHeadTrackLocked = (robotLockedTracks & (u8)AnimTrackFlag::HEAD_TRACK) != 0;
    if( isRobotHeadTrackLocked != AreAnyTracksLocked((u8)AnimTrackFlag::HEAD_TRACK) )
    {
      PRINT_NAMED_WARNING("MovementComponent.Update.HeadLockUnmatched",
                          "TrackRobot:%d, TrackEngineCount:%d",_robot.GetEnabledAnimationTracks(),_trackLockCount[GetFlagIndex((u8)AnimTrackFlag::HEAD_TRACK)]);
    }
    const bool isRobotLiftTrackLocked = (robotLockedTracks & (u8)AnimTrackFlag::LIFT_TRACK) != 0;
    if( isRobotLiftTrackLocked != AreAnyTracksLocked((u8)AnimTrackFlag::LIFT_TRACK) )
    {
      PRINT_NAMED_WARNING("MovementComponent.Update.LiftLockUnmatched",
                       "TrackRobot:%d, TrackEngineCount:%d",_robot.GetEnabledAnimationTracks(),_trackLockCount[GetFlagIndex((u8)AnimTrackFlag::LIFT_TRACK)]);
    }
    const bool isRobotBodyTrackLocked = (robotLockedTracks & (u8)AnimTrackFlag::BODY_TRACK) != 0;
    if( isRobotBodyTrackLocked != AreAnyTracksLocked((u8)AnimTrackFlag::BODY_TRACK) )
    {
      PRINT_NAMED_WARNING("MovementComponent.Update.BodyLockUnmatched",
                          "TrackRobot:%d, TrackEngineCount:%d",_robot.GetEnabledAnimationTracks(),_trackLockCount[GetFlagIndex((u8)AnimTrackFlag::BODY_TRACK)]);
    }
  }
  
  CheckForUnexpectedMovement(robotState);
}

void MovementComponent::CheckForUnexpectedMovement(const Cozmo::RobotState& robotState)
{
  // Disabling for sim robot due to odd behavior of measured wheel speeds and the lack
  // of wheel slip that is what allows this detection to work on the real robot
  if(!_robot.IsPhysical())
  {
    return;
  }
  
  // Don't check for unexpected movement while playing animations that are moving the body
  // because some of them rapidly move the motors which triggers false positives
  if(_robot.IsAnimating() && !AreAllTracksLocked((u8)AnimTrackFlag::BODY_TRACK))
  {
    return;
  }

  f32 lWheelSpeed_mmps = robotState.lwheel_speed_mmps;
  f32 rWheelSpeed_mmps = robotState.rwheel_speed_mmps;
  f32 zGyro_radps = robotState.rawGyroZ;
  
  // Don't check for unexpected movement when picked up, on charger, or while a cliff is detected
  if(robotState.status & (uint16_t)RobotStatusFlag::IS_PICKED_UP ||
     robotState.status & (uint16_t)RobotStatusFlag::IS_ON_CHARGER ||
     robotState.status & (uint16_t)RobotStatusFlag::CLIFF_DETECTED)
  {
    _unexpectedMovementCount = 0;
    return;
  }
  
  f32 speedDiff = rWheelSpeed_mmps - lWheelSpeed_mmps;
  
  // Outside velocity and inside velocity (outside velocity is the faster of the two)
  f32 vO = MAX(rWheelSpeed_mmps, lWheelSpeed_mmps);
  f32 vI = MIN(rWheelSpeed_mmps, lWheelSpeed_mmps);
  
  // Radius of the circle produced by the inside wheel during a turn
  f32 rR = vI*WHEEL_BASE_MM / (vO - vI);
  
  // Expected rotational velocity based on the two wheel speeds
  f32 omega = ABS(vO / (rR + WHEEL_BASE_MM));
  
  UnexpectedMovementType unexpectedMovementType = UnexpectedMovementType::TURNED_BUT_STOPPED;
  
  // Wheels aren't moving (using kMinWheelSpeed_mmps as a dead band)
  if(ABS(lWheelSpeed_mmps) + ABS(rWheelSpeed_mmps) < kMinWheelSpeed_mmps)
  {
    if(_unexpectedMovementCount > 0)
    {
      _unexpectedMovementCount--;
    }
    return;
  }
  
  // Gyro says we aren't turning
  if(NEAR(zGyro_radps, 0, kGyroTol_radps))
  {
    // But wheels say we are turning (the direction of the wheels are different like during a point turn)
    if(signbit(lWheelSpeed_mmps) != signbit(rWheelSpeed_mmps))
    {
      _unexpectedMovementCount++;
      unexpectedMovementType = UnexpectedMovementType::TURNED_BUT_STOPPED;
    }
    // Wheels are moving in same direction but the difference between what the gyro is reporting and what we expect
    // it to be reporting is too great
    // The expected rotational velocity, omega, is divided by 2 here due to the fact that the world isn't perfect
    // (lots of wheel slip and friction) so its tends to report velocities 2 times greater than what we are actually
    // measuring
    else if(ABS(ABS(omega*0.5f) - ABS(zGyro_radps)) > kExpectedVsActualGyroTol_radps)
    {
      _unexpectedMovementCount++;
      unexpectedMovementType = UnexpectedMovementType::TURNED_BUT_STOPPED;
    }
  }
  // Wheel speeds and gyro agree on the direction we are turning
  else if(signbit(speedDiff) == signbit(zGyro_radps))
  {
    // This check is for detecting if we are being turned in the direction we are already turning
    // this is difficult to detect due to physics. For example, we spin faster when carrying a block
    // than not carrying one. The surface we are on will also greatly affect this type of movement detection
    // for these reasons this is not enabled
    // With this commented out the case of being spun in the same direction will trigger TURNED_IN_OPPOSITE_DIRECTION
    // when we pass the target turn angle
//    if(!_robot.IsCarryingObject() &&
//       ABS(speedDiff) > kWheelDifForTurning_mmps &&
//       ABS(ABS(speedDiff) - ABS(zGyro_radps)*100) > 30 * MAX(ABS(speedDiff), 100.f) / 100.f)
//    {
//      _unexpectedMovementCount++;
//      unexpectedMovementType = UnexpectedMovementType::TURNED_IN_SAME_DIRECTION;
//    }
    if(_unexpectedMovementCount > 0)
    {
      _unexpectedMovementCount--;
      return;
    }
  }
  // Otherwise gyro says we are turning but our wheel speeds don't agree
  else
  {
    // Increment by 2 here to get this case to trigger twice as fast as other cases
    // The intuition is that this case is easier and more reliable to detect so there should not be any false positives
    _unexpectedMovementCount+=2;
    unexpectedMovementType = UnexpectedMovementType::TURNED_IN_OPPOSITE_DIRECTION;
  }
  
  if(_unexpectedMovementCount > kMaxUnexpectedMovementCount)
  {
    PRINT_NAMED_WARNING("MovementComponent.CheckForUnexpectedMovement",
                        "Unexpected movement detected %s",
                        EnumToString(unexpectedMovementType));
    
    _unexpectedMovementCount = 0;
    
    _robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::UnexpectedMovement(robotState.timestamp, unexpectedMovementType)));
  }
}

void MovementComponent::RemoveFaceLayerWhenHeadMoves(AnimationStreamer::Tag faceLayerTag, TimeStamp_t duration_ms)
{
  PRINT_NAMED_DEBUG("MovementComponent.RemoveFaceLayersWhenHeadMoves.",
                    "Registering tag=%d for removal with duration=%dms",
                    faceLayerTag, duration_ms);

  FaceLayerToRemove info{
    .duration_ms  = duration_ms,
    .headWasMoving = _isHeadMoving,
  };
  _faceLayerTagsToRemoveOnHeadMovement[faceLayerTag] = std::move(info);
  
}

  
template<>
void MovementComponent::HandleMessage(const ExternalInterface::DriveWheels& msg)
{
  if(_ignoreDirectDrive)
  {
    PRINT_NAMED_INFO("MovementComponent.EventHandler.DriveWheels",
                     "Ignoring DriveWheels message while direct drive is disabled");
    return;
  }
  
  if(!_drivingWheels && AreAnyTracksLocked((u8)AnimTrackFlag::BODY_TRACK)) {
    PRINT_NAMED_INFO("MovementComponent.EventHandler.DriveWheels.WheelsLocked",
                     "Ignoring ExternalInterface::DriveWheels while wheels are locked.");
  } else {
    DirectDriveCheckSpeedAndLockTracks(ABS(msg.lwheel_speed_mmps) + ABS(msg.rwheel_speed_mmps),
                                       _drivingWheels,
                                       (u8)AnimTrackFlag::BODY_TRACK);
    _robot.SendRobotMessage<RobotInterface::DriveWheels>(msg.lwheel_speed_mmps, msg.rwheel_speed_mmps,
                                                         msg.lwheel_accel_mmps2, msg.rwheel_accel_mmps2);
  }
}

template<>
void MovementComponent::HandleMessage(const ExternalInterface::TurnInPlaceAtSpeed& msg)
{
  if(_ignoreDirectDrive)
  {
    PRINT_NAMED_INFO("MovementComponent.EventHandler.TurnInPlaceAtSpeed",
                     "Ignoring TurnInPlaceAtSpeed message while direct drive is disabled");
    return;
  }
  
  if(!_drivingWheels && AreAnyTracksLocked((u8)AnimTrackFlag::BODY_TRACK)) {
    PRINT_NAMED_INFO("MovementComponent.EventHandler.TurnInPlaceAtSpeed.WheelsLocked",
                     "Ignoring ExternalInterface::TurnInPlaceAtSpeed while wheels are locked.");
  } else {
    f32 turnSpeed = msg.speed_rad_per_sec;
    if (std::fabsf(turnSpeed) > MAX_BODY_ROTATION_SPEED_RAD_PER_SEC) {
      PRINT_NAMED_WARNING("MovementComponent.EventHandler.TurnInPlaceAtSpeed.SpeedExceedsLimit",
                          "Speed of %f deg/s exceeds limit of %f deg/s. Clamping.",
                          RAD_TO_DEG_F32(turnSpeed), MAX_BODY_ROTATION_SPEED_DEG_PER_SEC);
      turnSpeed = std::copysign(MAX_BODY_ROTATION_SPEED_RAD_PER_SEC, turnSpeed);
    }
    DirectDriveCheckSpeedAndLockTracks(turnSpeed, _drivingWheels, (u8)AnimTrackFlag::BODY_TRACK);
    _robot.SendRobotMessage<RobotInterface::TurnInPlaceAtSpeed>(turnSpeed, msg.accel_rad_per_sec2);
  }
}

template<>
void MovementComponent::HandleMessage(const ExternalInterface::MoveHead& msg)
{
  if(_ignoreDirectDrive)
  {
    PRINT_NAMED_INFO("MovementComponent.EventHandler.MoveHead",
                     "Ignoring MoveHead message while direct drive is disabled");
    return;
  }
  
  if(!_drivingHead && AreAnyTracksLocked((u8)AnimTrackFlag::HEAD_TRACK)) {
    PRINT_NAMED_INFO("MovementComponent.EventHandler.MoveHead.HeadLocked",
                     "Ignoring ExternalInterface::MoveHead while head is locked.");
  } else {
    DirectDriveCheckSpeedAndLockTracks(msg.speed_rad_per_sec, _drivingHead, (u8)AnimTrackFlag::HEAD_TRACK);
    _robot.SendRobotMessage<RobotInterface::MoveHead>(msg.speed_rad_per_sec);
  }
}

template<>
void MovementComponent::HandleMessage(const ExternalInterface::MoveLift& msg)
{
  if(_ignoreDirectDrive)
  {
    PRINT_NAMED_INFO("MovementComponent.EventHandler.MoveLift",
                     "Ignoring MoveLift message while direct drive is disabled");
    return;
  }
  
  if(!_drivingLift && AreAnyTracksLocked((u8)AnimTrackFlag::LIFT_TRACK)) {
    PRINT_NAMED_INFO("MovementComponent.EventHandler.MoveLift.LiftLocked",
                     "Ignoring ExternalInterface::MoveLift while lift is locked.");
  } else {
    DirectDriveCheckSpeedAndLockTracks(msg.speed_rad_per_sec, _drivingLift, (u8)AnimTrackFlag::LIFT_TRACK);
    _robot.SendRobotMessage<RobotInterface::MoveLift>(msg.speed_rad_per_sec);
  }
}

template<>
void MovementComponent::HandleMessage(const ExternalInterface::DriveArc& msg)
{
  if(_ignoreDirectDrive)
  {
    PRINT_NAMED_INFO("MovementComponent.EventHandler.DriveArc",
                     "Ignoring DriveArc message while direct drive is disabled");
    return;
  }
  
  if(!_drivingWheels && AreAnyTracksLocked((u8)AnimTrackFlag::BODY_TRACK)) {
    PRINT_NAMED_INFO("MovementComponent.EventHandler.DriveArc.WheelsLocked",
                     "Ignoring ExternalInterface::DriveArc while wheels are locked.");
  } else {
    DirectDriveCheckSpeedAndLockTracks(msg.speed_mmps,
                                       _drivingWheels,
                                       (u8)AnimTrackFlag::BODY_TRACK);
    _robot.SendRobotMessage<RobotInterface::DriveWheelsCurvature>(msg.speed_mmps,
                                                                  DEFAULT_PATH_MOTION_PROFILE.accel_mmps2,
                                                                  DEFAULT_PATH_MOTION_PROFILE.decel_mmps2,
                                                                  msg.curvatureRadius_mm);
  }
}

template<>
void MovementComponent::HandleMessage(const ExternalInterface::StopAllMotors& msg)
{
  StopAllMotors();
}

void MovementComponent::DirectDriveCheckSpeedAndLockTracks(f32 speed, bool& flag, u8 tracks)
{
  if(NEAR_ZERO(speed))
  {
    flag = false;
    if(AreAllTracksLocked(tracks))
    {
      UnlockTracks(tracks);
    }
  }
  else
  {
    flag = true;
    if(!AreAllTracksLocked(tracks))
    {
      LockTracks(tracks);
    }
  }
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
  // If we are direct driving then make sure to unlock tracks and set flags appropriately
  if(IsDirectDriving())
  {
    DirectDriveCheckSpeedAndLockTracks(0, _drivingHead,   (u8)AnimTrackFlag::HEAD_TRACK);
    DirectDriveCheckSpeedAndLockTracks(0, _drivingLift,   (u8)AnimTrackFlag::LIFT_TRACK);
    DirectDriveCheckSpeedAndLockTracks(0, _drivingWheels, (u8)AnimTrackFlag::BODY_TRACK);
  }
  
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

AnimTrackFlag MovementComponent::GetFlagFromIndex(int index)
{
  return (AnimTrackFlag)((u32)1 << index);
}
  
bool MovementComponent::AreAnyTracksLocked(u8 tracks) const
{
  for(int i = 0; i < (int)AnimConstants::NUM_TRACKS; i++)
  {
    if((tracks & 1) && (_trackLockCount[i] > 0))
    {
      return true;
    }
    tracks = tracks >> 1;
  }
  return false;
}

bool MovementComponent::AreAllTracksLocked(u8 tracks) const
{
  if(tracks == 0)
  {
    PRINT_NAMED_WARNING("MovementComponent.AreAllTracksLocked", "All of the NO_TRACKS are not locked");
    return false;
  }
  for(int i = 0; i < (int)AnimConstants::NUM_TRACKS; i++)
  {
    if((tracks & 1) && (_trackLockCount[i] == 0))
    {
      return false;
    }
    tracks = tracks >> 1;
  }
  return true;
}

void MovementComponent::CompletelyUnlockAllTracks()
{
  for(int i = 0; i < (int)AnimConstants::NUM_TRACKS; i++)
  {
    // Repeatedly unlock the track until its lock count is zero 
    while(_trackLockCount[i] > 0)
    {
      PRINT_NAMED_INFO("MovementComponent.UnlockAllTracks",
                       "Unlocking track %s",
                       EnumToString(GetFlagFromIndex(i)));
      UnlockTracks((u8)GetFlagFromIndex(i));
    }
  }
}

void MovementComponent::LockTracks(uint8_t tracks)
{
  for (int i=0; i < (int)AnimConstants::NUM_TRACKS; i++)
  {
    uint8_t curTrack = (1 << i);
    if ((tracks & curTrack) == curTrack)
    {
      ++_trackLockCount[i];
      
      // If we just went from not locked to locked, inform the robot
      if (_trackLockCount[i] == 1)
      {
        _robot.SendMessage(RobotInterface::EngineToRobot(AnimKeyFrame::DisableAnimTracks(curTrack)));
      }
    }
  }
  if(DEBUG_ANIMATION_LOCKING) {
    PRINT_NAMED_INFO("MovementComponent.LockTracks", "locked: (0x%x) %s, result:",
                     tracks,
                     AnimTrackHelpers::AnimTrackFlagsToString(tracks).c_str());
    PrintLockState();
  }
}

void MovementComponent::UnlockTracks(uint8_t tracks)
{
  for (int i=0; i < (int)AnimConstants::NUM_TRACKS; i++)
  {
    uint8_t curTrack = (1 << i);
    if ((tracks & curTrack) == curTrack)
    {
      --_trackLockCount[i];

      // If we just went from locked to not locked, inform the robot
      if (_trackLockCount[i] == 0)
      {
        _robot.SendMessage(RobotInterface::EngineToRobot(AnimKeyFrame::EnableAnimTracks(curTrack)));
      }

      // It doesn't matter if there are more unlocks than locks
      if(_trackLockCount[i] < 0)
      {
        PRINT_NAMED_WARNING("MovementComponent.UnlockTracks", "Track locks and unlocks do not match");
        _trackLockCount[i] = 0;
      }
    }
  }
  if(DEBUG_ANIMATION_LOCKING) {
    PRINT_NAMED_INFO("MovementComponent.UnlockTracks", "unlocked: (0x%x) %s, result:",
                     tracks,
                     AnimTrackHelpers::AnimTrackFlagsToString(tracks).c_str());
    PrintLockState();
  }
}

void MovementComponent::PrintLockState() const
{
  std::stringstream ss;
  for( int trackNum = 0; trackNum < (int)AnimConstants::NUM_TRACKS; ++trackNum ) {
    if( _trackLockCount[trackNum] > 0 ) {
      uint8_t trackEnumVal = 1 << trackNum;
      ss << AnimTrackHelpers::AnimTrackFlagsToString(trackEnumVal) << ":" << _trackLockCount[trackNum] << ' ';
    }
  }

  PRINT_NAMED_DEBUG("MovementComponent.Locks", "%s", ss.str().c_str());
}

} // namespace Cozmo
} // namespace Anki
