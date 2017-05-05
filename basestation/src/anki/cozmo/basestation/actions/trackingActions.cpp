/**
 * File: trackingActions.h
 *
 * Author: Andrew Stein
 * Date:   12/11/2015
 *
 * Description: Defines an interface and specific actions for tracking, derived
 *              from the general IAction interface.
 *
 *
 * Copyright: Anki, Inc. 2015
 **/


#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/trackingActions.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/movementComponent.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/drivingAnimationHandler.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/petWorld.h"
#include "anki/cozmo/basestation/robot.h"

#include "clad/externalInterface/messageEngineToGameTag.h"
#include "clad/externalInterface/messageEngineToGame.h"

#include "anki/common/basestation/utils/timer.h"

#include "util/math/math.h"

#define DEBUG_TRACKING_ACTIONS 0

namespace Anki {
namespace Cozmo {
  
static const char * const kLogChannelName = "Actions";
  
  
#pragma mark -
#pragma mark ITrackAction
  
ITrackAction::ITrackAction(Robot& robot, const std::string name, const RobotActionType type)
: IAction(robot,
          name,
          type,
          ((u8)AnimTrackFlag::BODY_TRACK | (u8)AnimTrackFlag::HEAD_TRACK))
, _eyeShiftTag(AnimationStreamer::NotAnimatingTag)
, _originalEyeDartDist(-1.f)
{

}

ITrackAction::~ITrackAction()
{
  if(_eyeShiftTag != AnimationStreamer::NotAnimatingTag) {
    // Make sure any eye shift gets removed
    _robot.GetAnimationStreamer().RemovePersistentFaceLayer(_eyeShiftTag);
    _eyeShiftTag = AnimationStreamer::NotAnimatingTag;
  }

  if(_originalEyeDartDist >= 0.f) {
    // Make sure to restore original eye dart distance
    _robot.GetAnimationStreamer().SetParam(LiveIdleAnimationParameter::EyeDartMaxDistance_pix, _originalEyeDartDist);
  }
  
  // Make sure we abort any sound actions we triggered
  _robot.GetActionList().Cancel(_soundAnimTag);
  
  if(HasStarted())
  {
    // Make sure we don't leave the head/body moving
    switch(_mode)
    {
      case Mode::HeadAndBody:
        _robot.GetMoveComponent().StopBody();
        _robot.GetMoveComponent().StopHead();
        break;
        
      case Mode::BodyOnly:
        _robot.GetMoveComponent().StopBody();
        break;
        
      case Mode::HeadOnly:
        _robot.GetMoveComponent().StopHead();
        break;
    }
  }
  
  _robot.GetDrivingAnimationHandler().ActionIsBeingDestroyed();
}

// TODO:(bn) if we implemented a parallel compound action function like "Stop on first action complete"
// instead of the current behavior of "stop when all actions are complete", I don't think we'd need this
// anymore
void ITrackAction::StopTrackingWhenOtherActionCompleted( u32 otherActionTag )
{
  if( HasStarted() ) {
    if( otherActionTag != ActionConstants::INVALID_TAG &&
        ! IsTagInUse( otherActionTag ) ) {
      PRINT_NAMED_WARNING("ITrackAction.SetOtherAction.InvalidOtherActionTag",
                          "[%d] trying to set tag %d, but it is not in use. Keeping tag as old value of %d",
                          GetTag(),
                          otherActionTag,
                          _stopOnOtherActionTag);
    }
    else {
      // This means we are changing the tag while we are running, which is a bit weird but should work as long
      // as the action is valid (or INVALID_TAG)

      if( otherActionTag == ActionConstants::INVALID_TAG ) {
        PRINT_CH_INFO(kLogChannelName, "ITrackAction.StopTrackingOnOtherAction.Clear",
                      "[%d] Was waiting on action %d to stop, now will hang",
                      GetTag(),
                      _stopOnOtherActionTag);
      }
      else {
        PRINT_CH_INFO(kLogChannelName, "ITrackAction.StopTrackingOnOtherAction.SetWhileRunning",
                      "[%d] Will stop this action when %d completes",
                      GetTag(),
                      otherActionTag);
      }
      
      _stopOnOtherActionTag = otherActionTag;
    }
  }
  else {
    // this action will be checked in Init to see if it is in use (it is done there so it can cause the action
    // to fail), so don't do anything with it now
    PRINT_CH_INFO(kLogChannelName, "ITrackAction.StopTrackingOnOtherAction.Set",
                  "[%d] Will stop this action when %d completes",
                  GetTag(),
                  otherActionTag);
    _stopOnOtherActionTag = otherActionTag;
  }
}
 
void ITrackAction::SetPanDuration(f32 panDuration_sec)
{
  DEV_ASSERT(!HasStarted(), "ITrackAction.SetPanDuration.ActionAlreadyStarted");
  _panDuration_sec = panDuration_sec;
}

void ITrackAction::SetTiltDuration(f32 tiltDuration_sec)
{
  DEV_ASSERT(!HasStarted(), "ITrackAction.SetTiltDuration.ActionAlreadyStarted");
  _tiltDuration_sec = tiltDuration_sec;
}

void ITrackAction::SetUpdateTimeout(float timeout_sec)
{
  DEV_ASSERT(!HasStarted(), "ITrackAction.SetUpdateTimeout.ActionAlreadyStarted");
  _updateTimeout_sec = timeout_sec;
}
  
void ITrackAction::SetDesiredTimeToReachTarget(f32 time_sec)
{
  DEV_ASSERT(!HasStarted(), "ITrackAction.SetDesiredTimeToReachTarget.ActionAlreadyStarted");
  _timeToReachTarget_sec = time_sec;
}

void ITrackAction::EnableDrivingAnimation(bool enable)
{
  DEV_ASSERT(!HasStarted(), "ITrackAction.EnableDrivingAnimation.ActionAlreadyStarted");
  _shouldPlayDrivingAnimation = enable;
}

void ITrackAction::SetSound(const AnimationTrigger animName)
{
  DEV_ASSERT(!HasStarted(), "ITrackAction.SetSound.ActionAlreadyStarted");
  _turningSoundAnimTrigger = animName;
}

void ITrackAction::SetMinPanAngleForSound(const Radians& angle)
{
  DEV_ASSERT(!HasStarted(), "ITrackAction.SetMinPanAngleForSound.ActionAlreadyStarted");
  _minPanAngleForSound = angle.getAbsoluteVal();
}

void ITrackAction::SetMinTiltAngleForSound(const Radians& angle)
{
  DEV_ASSERT(!HasStarted(), "ITrackAction.SetMinTiltAngleForSound.ActionAlreadyStarted");
  _minTiltAngleForSound = angle.getAbsoluteVal();
}

void ITrackAction::SetClampSmallAnglesToTolerances(bool tf)
{
  DEV_ASSERT(!HasStarted(), "ITrackAction.SetClampSmallAnglesToTolerances.ActionAlreadyStarted");
  _clampSmallAngles = tf;
}

void ITrackAction::SetClampSmallAnglesPeriod(float min_sec, float max_sec)
{
  DEV_ASSERT(!HasStarted(), "ITrackAction.SetClampSmallAnglesPeriod.ActionAlreadyStarted");
  _clampSmallAnglesMinPeriod_s = min_sec;
  _clampSmallAnglesMaxPeriod_s = max_sec;
}
  
void ITrackAction::SetMaxHeadAngle(const Radians& maxHeadAngle_rads)
{
  DEV_ASSERT(!HasStarted(), "ITrackAction.SetMaxHeadAngle.ActionAlreadyStarted");
  _maxHeadAngle = maxHeadAngle_rads;
}

void ITrackAction::SetMoveEyes(bool moveEyes)
{
  DEV_ASSERT(!HasStarted(), "ITrackAction.SetMoveEyes.ActionAlreadyStarted");
  _moveEyes = moveEyes;
}

void ITrackAction::SetSoundSpacing(f32 spacingMin_sec, f32 spacingMax_sec)
{
  DEV_ASSERT(!HasStarted(), "ITrackAction.SetSoundSpacing.ActionAlreadyStarted");
  _soundSpacingMin_sec = spacingMin_sec;
  _soundSpacingMax_sec = spacingMax_sec;
}

void ITrackAction::SetStopCriteria(const Radians& panTol, const Radians& tiltTol,
                                   f32 minDist_mm, f32 maxDist_mm, f32 time_sec)
{
  DEV_ASSERT(!HasStarted(), "ITrackAction.SetStopCriteria.ActionAlreadyStarted");
  _stopCriteria.panTol       = panTol;
  _stopCriteria.tiltTol      = tiltTol;
  _stopCriteria.minDist_mm   = minDist_mm;
  _stopCriteria.maxDist_mm   = maxDist_mm;
  _stopCriteria.duration_sec = time_sec;
  
  _stopCriteria.withinTolSince_sec = -1.f;
}
  
void ITrackAction::SetMode(Mode newMode)
{
  DEV_ASSERT(!HasStarted(), "ITrackAction.SetMode.ActionAlreadyStarted");
  _mode = newMode;
  if(GetState() == ActionResult::NOT_STARTED)
  {
    switch(_mode)
    {
      case Mode::HeadAndBody:
        SetTracksToLock((u8)AnimTrackFlag::BODY_TRACK | (u8)AnimTrackFlag::HEAD_TRACK);
        break;
      case Mode::HeadOnly:
        SetTracksToLock((u8)AnimTrackFlag::HEAD_TRACK);
        break;
      case Mode::BodyOnly:
        SetTracksToLock((u8)AnimTrackFlag::BODY_TRACK);
        break;
    }
  }
  else {
    PRINT_NAMED_WARNING("ITrackAction.SetMode.AlreadyRunning",
                        "[%d] Trying to set tracking mode, but action is running so track locking will be wrong",
                        GetTag());
  }
}
  
void ITrackAction::SetPanTolerance(const Radians& panThreshold)
{
  DEV_ASSERT(!HasStarted(), "ITrackAction.SetPanTolerance.ActionAlreadyStarted");
  _panTolerance = panThreshold.getAbsoluteVal();
  
  // NOTE: can't be lower than what is used internally on the robot
  if( _panTolerance.ToFloat() < POINT_TURN_ANGLE_TOL ) {
    PRINT_NAMED_WARNING("ITrackAction.InvalidTolerance",
                        "Tried to set tolerance of %fdeg, min is %f",
                        _panTolerance.getDegrees(),
                        RAD_TO_DEG(POINT_TURN_ANGLE_TOL));
    _panTolerance = POINT_TURN_ANGLE_TOL;
  }
}

  
void ITrackAction::SetTiltTolerance(const Radians& tiltThreshold)
{
  DEV_ASSERT(!HasStarted(), "ITrackAction.SetTiltTolerance.ActionAlreadyStarted");
  _tiltTolerance = tiltThreshold.getAbsoluteVal();
  
  // NOTE: can't be lower than what is used internally on the robot
  if( _tiltTolerance.ToFloat() < HEAD_ANGLE_TOL ) {
    PRINT_NAMED_WARNING("ITrackAction.InvalidTolerance",
                        "Tried to set tolerance of %fdeg, min is %f",
                        _tiltTolerance.getDegrees(),
                        RAD_TO_DEG(HEAD_ANGLE_TOL));
    _tiltTolerance = HEAD_ANGLE_TOL;
  }
}

ActionResult ITrackAction::Init()
{
  if(_shouldPlayDrivingAnimation)
  {
    const bool kLoopWithoutPathToFollow = true;
    _robot.GetDrivingAnimationHandler().Init(GetTracksToLock(), GetTag(), IsSuppressingTrackLocking(),
                                             kLoopWithoutPathToFollow);
  }
  
  // Store eye dart setting so we can restore after tracking
  _originalEyeDartDist = _robot.GetAnimationStreamer().GetParam(LiveIdleAnimationParameter::EyeDartMaxDistance_pix);
  
  // Reduce eye darts so we better appear to be tracking and not look around
  _robot.GetAnimationStreamer().SetParam(LiveIdleAnimationParameter::EyeDartMaxDistance_pix, 1.f);

  if( _stopOnOtherActionTag != ActionConstants::INVALID_TAG &&
      ! IsTagInUse( _stopOnOtherActionTag ) ) {
    PRINT_NAMED_WARNING("ITrackAction.Init.InvalidOtherActionTag",
                        "[%d] Waiting on tag %d to stop this action, but that tag is no longer in use. Stopping now",
                        GetTag(),
                        _stopOnOtherActionTag);
    return ActionResult::ABORT;
  }
  
  _lastUpdateTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  const ActionResult result = InitInternal();
  if(ActionResult::SUCCESS == result && _shouldPlayDrivingAnimation)
  {
    _robot.GetDrivingAnimationHandler().PlayStartAnim();
  }
  return result;
}
  
bool ITrackAction::InterruptInternal()
{
  _lastUpdateTime = 0.0f;
  return true;
}

ActionResult ITrackAction::CheckIfDoneReturnHelper(ActionResult result)
{
  if(_shouldPlayDrivingAnimation)
  {
    _robot.GetDrivingAnimationHandler().PlayEndAnim();
    _finalActionResult = result; // This will get returned once the end anim completes
    return ActionResult::RUNNING;
  }
  else
  {
    return result;
  }
}
  
ActionResult ITrackAction::CheckIfDone()
{
  if(_shouldPlayDrivingAnimation)
  {
    if(_robot.GetDrivingAnimationHandler().IsPlayingEndAnim())
    {
      return ActionResult::RUNNING;
    }
    else if(_robot.GetDrivingAnimationHandler().HasFinishedEndAnim())
    {
      DEV_ASSERT(_finalActionResult != ActionResult::NOT_STARTED, "ITrackAction.CheckIfDone.FinalActionResultNotSet");
      return _finalActionResult;
    }
  }
  
  if(_stopOnOtherActionTag != ActionConstants::INVALID_TAG &&
     !IsTagInUse(_stopOnOtherActionTag) )
  {
    PRINT_CH_INFO(kLogChannelName, "ITrackAction.FinishedByOtherAction",
                  "[%d] action %s stopping because we were told to stop when another action stops (and it did)",
                  GetTag(),
                  GetName().c_str());
    
    return CheckIfDoneReturnHelper(ActionResult::SUCCESS);
  }
  
  const f32 currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  // See if there are new absolute pan/tilt angles from the derived class
  Radians absPanAngle = 0, absTiltAngle = 0;
  f32 distance_mm = 0.f;
  const UpdateResult updateResult = UpdateTracking(absPanAngle, absTiltAngle, distance_mm);
  switch(updateResult)
  {
    case UpdateResult::NewInfo:
    case UpdateResult::PredictedInfo:
    {
      if( absTiltAngle > _maxHeadAngle ) {
        absTiltAngle = _maxHeadAngle;
      }
      
      // Record latest update to avoid timing out
      if(_updateTimeout_sec > 0.0f) {
        _lastUpdateTime = currentTime;
      }
      
      if(DEBUG_TRACKING_ACTIONS) {
        PRINT_NAMED_INFO("ITrackAction.CheckIfDone.NewInfo",
                         "Commanding %sabs angles: pan=%.1fdeg, tilt=%.1fdeg, dist=%1.fmm",
                         updateResult == UpdateResult::PredictedInfo ? "predicted " : "",
                         absPanAngle.getDegrees(), absTiltAngle.getDegrees(), distance_mm);
      }
      
      bool angleLargeEnoughForSound = false;
      f32  eyeShiftX = 0.f, eyeShiftY = 0.f;
      
      // Tilt Head:
      f32 relTiltAngle = (absTiltAngle - _robot.GetHeadAngle()).ToFloat();
      
      const bool shouldClampSmallAngles = UpdateSmallAngleClamping();
        
      // If enabled, always move at least the tolerance amount
      if(shouldClampSmallAngles && FLT_LE(std::abs(relTiltAngle), _tiltTolerance.ToFloat()))
      {
        relTiltAngle = std::copysign(_tiltTolerance.ToFloat(), relTiltAngle);
        absTiltAngle = _robot.GetHeadAngle() + relTiltAngle;
      }
      
      if((Mode::HeadAndBody == _mode || Mode::HeadOnly == _mode) &&
         FLT_GE(std::abs(relTiltAngle), _tiltTolerance.ToFloat()))
      {
        const f32 speed = std::abs(relTiltAngle) / _tiltDuration_sec;
        const f32 accel = MAX_HEAD_ACCEL_RAD_PER_S2;
        
        if(RESULT_OK != _robot.GetMoveComponent().MoveHeadToAngle(absTiltAngle.ToFloat(), speed, accel))
        {
          return CheckIfDoneReturnHelper(ActionResult::SEND_MESSAGE_TO_ROBOT_FAILED);
        }
        
        if(std::abs(relTiltAngle) > _minTiltAngleForSound) {
          angleLargeEnoughForSound = true;
        }
        
        if(_moveEyes) {
          const f32 y_mm = std::tan(-relTiltAngle) * HEAD_CAM_POSITION[0];
          eyeShiftY = y_mm * (static_cast<f32>(ProceduralFace::HEIGHT/2) / SCREEN_SIZE[1]);
        }
      }
      
      // Pan Body:
      f32 relPanAngle = (absPanAngle - _robot.GetPose().GetRotation().GetAngleAroundZaxis()).ToFloat();
      
      const bool isPanWithinTol = Util::IsFltLE(std::abs(relPanAngle), _panTolerance.ToFloat());
      // If enabled, always move at least the tolerance amount
      if(shouldClampSmallAngles && isPanWithinTol)
      {
        relPanAngle = std::copysign(_panTolerance.ToFloat(), relPanAngle);
        absPanAngle = _robot.GetPose().GetRotation().GetAngleAroundZaxis().ToFloat() + relPanAngle;
      }
      
      // If distance is non-zero and the body is allowed to move based on mode, then we need to move
      // forward (fwd) or backward (bwd)
      const bool needToMoveFwdBwd = (_mode != Mode::HeadOnly) && !Util::IsNearZero(distance_mm);
      
      // If the relative pan angle is greater than the tolerance, we need to pan
      const bool needToPan = Util::IsFltGE(std::abs(relPanAngle), _panTolerance.ToFloat());
      
      if((Mode::HeadAndBody == _mode || Mode::BodyOnly == _mode) && (needToMoveFwdBwd || needToPan))
      {
        const f32 kMaxPanAngle_deg = 89.f;
        
        if(needToMoveFwdBwd)
        {
          s16 radius = std::numeric_limits<s16>::max(); // default: drive straight
          if(!isPanWithinTol)
          {
            // Set wheel speeds to drive an arc to the salient point. Note: use the *relative* angle for this!
            const f32 denomAngle = std::min(std::abs(relPanAngle), DEG_TO_RAD(kMaxPanAngle_deg));
            const f32 d = distance_mm / std::cosf(denomAngle);
            const f32 d2 = d*d;
            const f32 radiusDenom = 2.f*std::sqrtf(d2 - distance_mm*distance_mm);
            radius = std::round( std::copysign( (d2 / radiusDenom), relPanAngle) );
          }
          
          // Specify a fixed duration to reach the goal and compute speed from it
          const f32 wheelspeed_mmps = std::min(MAX_WHEEL_SPEED_MMPS, distance_mm / _timeToReachTarget_sec);
          const f32 accel = MAX_WHEEL_ACCEL_MMPS2; // Expose?
          
          if(DEBUG_TRACKING_ACTIONS) {
            PRINT_CH_DEBUG(kLogChannelName, "ITrackAction.CheckIfDone.DriveWheelsCurvature",
                           "d=%f r=%hd relPan=%.1fdeg speed=%f accel=%f",
                           distance_mm, radius, RAD_TO_DEG(relPanAngle), wheelspeed_mmps, accel);
          }
          
          Result result = _robot.SendRobotMessage<RobotInterface::DriveWheelsCurvature>(wheelspeed_mmps, accel, radius);
          
          if(RESULT_OK != result) {
            return CheckIfDoneReturnHelper(ActionResult::SEND_MESSAGE_TO_ROBOT_FAILED);
          }
          
        }
        else
        {
          // Get rotation angle around drive center
          Pose3d rotatedPose;
          Pose3d dcPose = _robot.GetDriveCenterPose();
          dcPose.SetRotation(absPanAngle, Z_AXIS_3D());
          _robot.ComputeOriginPose(dcPose, rotatedPose);
          
          const Radians& turnAngle = rotatedPose.GetRotation().GetAngleAroundZaxis();

          // Just turn in place
          const f32 rotSpeed_radPerSec = std::min(MAX_BODY_ROTATION_SPEED_RAD_PER_SEC, std::abs(relPanAngle) / _panDuration_sec);
          const f32 accel = MAX_BODY_ROTATION_ACCEL_RAD_PER_SEC2;
          
          if(DEBUG_TRACKING_ACTIONS) {
            PRINT_CH_DEBUG(kLogChannelName, "ITrackAction.CheckIfDone.SetBodyAngle",
                           "d=%f relPan=%.1fdeg speed=%f accel=%f",
                           distance_mm, RAD_TO_DEG(relPanAngle), rotSpeed_radPerSec, accel);
          }
          
          RobotInterface::SetBodyAngle setBodyAngle(turnAngle.ToFloat(),      // angle_rad
                                                    rotSpeed_radPerSec,       // max_speed_rad_per_sec
                                                    accel,                    // accel_rad_per_sec2
                                                    _panTolerance.ToFloat(),  // angle_tolerance
                                                    0,                        // num_half_revolutions
                                                    true);                    // use_shortest_direction
          
          if(RESULT_OK != _robot.SendRobotMessage<RobotInterface::SetBodyAngle>(std::move(setBodyAngle))) {
            return CheckIfDoneReturnHelper(ActionResult::SEND_MESSAGE_TO_ROBOT_FAILED);
          }
        }
        
        if(std::abs(relPanAngle) > _minPanAngleForSound) {
          angleLargeEnoughForSound = true;
        }
        
        if(_moveEyes) {
          // Compute horizontal eye movement
          // Note: assuming screen is about the same x distance from the neck joint as the head cam
          const f32 x_mm = std::tan(relPanAngle) * HEAD_CAM_POSITION[0];
          eyeShiftX = x_mm * (static_cast<f32>(ProceduralFace::WIDTH/2) / SCREEN_SIZE[0]);
        }
      }
      
      // Play sound if it's time and either angle was big enough
      const bool haveTurningSoundAnim = (AnimationTrigger::Count != _turningSoundAnimTrigger);
      if(haveTurningSoundAnim && (currentTime > _nextSoundTime) && angleLargeEnoughForSound)
      {
        // Queue sound to only play if nothing else is playing
        PlayAnimationAction* soundAction = new TriggerLiftSafeAnimationAction(_robot, _turningSoundAnimTrigger, 1, false);
        _soundAnimTag = soundAction->GetTag();
        _robot.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, soundAction);
        
        _nextSoundTime = currentTime + Util::numeric_cast<float>(GetRNG().RandDblInRange(_soundSpacingMin_sec, _soundSpacingMax_sec));
      }
      
      // Move eyes if indicated
      if(_moveEyes && (eyeShiftX != 0.f || eyeShiftY != 0.f))
      {
        // Clip, but retain sign
        eyeShiftX = CLIP(eyeShiftX, (f32)-ProceduralFace::WIDTH/4,  (f32)ProceduralFace::WIDTH/4);
        eyeShiftY = CLIP(eyeShiftY, (f32)-ProceduralFace::HEIGHT/4, (f32)ProceduralFace::HEIGHT/4);
        
        if(DEBUG_TRACKING_ACTIONS) {
          PRINT_NAMED_DEBUG("ITrackAction.CheckIfDone.EyeShift",
                            "Adjusting eye shift to (%.1f,%.1f), with tag=%d",
                            eyeShiftX, eyeShiftY, _eyeShiftTag);
        }
        
        // Expose as params?
        const f32 kMaxLookUpScale   = 1.1f;
        const f32 kMinLookDownScale = 0.8f;
        const f32 kOuterEyeScaleIncrease = 0.1f;
        
        ProceduralFace procFace;
        procFace.LookAt(eyeShiftX, eyeShiftY,
                        static_cast<f32>(ProceduralFace::WIDTH/4),
                        static_cast<f32>(ProceduralFace::HEIGHT/4),
                        kMaxLookUpScale, kMinLookDownScale, kOuterEyeScaleIncrease);
        
        if(_eyeShiftTag == AnimationStreamer::NotAnimatingTag) {
          // Start a new eye shift layer
          AnimationStreamer::FaceTrack faceTrack;
          faceTrack.AddKeyFrameToBack(ProceduralFaceKeyFrame(procFace, BS_TIME_STEP));
          _eyeShiftTag = _robot.GetAnimationStreamer().AddPersistentFaceLayer("TrackActionEyeShift", std::move(faceTrack));
        } else {
          // Augment existing eye shift layer
          _robot.GetAnimationStreamer().AddToPersistentFaceLayer(_eyeShiftTag, ProceduralFaceKeyFrame(procFace, BS_TIME_STEP));
        }
      } // if(_moveEyes)
      
      // Can't meet stop criteria based on predicted updates (as opposed to actual observations)
      if(updateResult != UpdateResult::PredictedInfo)
      {
        const bool shouldStop = StopCriteriaMetAndTimeToStop(relPanAngle, relTiltAngle, distance_mm, currentTime);
        if(shouldStop)
        {
          return CheckIfDoneReturnHelper(ActionResult::SUCCESS);
        }
      }
      
      break;
    }

      
    case UpdateResult::ShouldStop:
    {
      // Stop immediately. Yes, the destructor will also do this, but if we have driving animations, we may
      // return RUNNING for a bit while those finish and we want to make sure to stop _now_ if the UpdateTracking
      // function said we should.
      switch(_mode)
      {
        case Mode::HeadAndBody:
          _robot.GetMoveComponent().StopHead();
          _robot.GetMoveComponent().StopBody();
          break;
          
        case Mode::HeadOnly:
          _robot.GetMoveComponent().StopHead();
          break;
          
        case Mode::BodyOnly:
          _robot.GetMoveComponent().StopBody();
          break;
      }
      
      // NOTE: Intentional fall-through to NoNewInfo case below so we check for timeout
    }
      
      
    case UpdateResult::NoNewInfo:
    {
      // Didn't get an observation, see if we haven't had new info in awhile
      if(_updateTimeout_sec > 0.0f && _lastUpdateTime > 0.0f)
      {
        if(currentTime - _lastUpdateTime > _updateTimeout_sec) {
          PRINT_CH_INFO(kLogChannelName, "ITrackAction.CheckIfDone.Timeout",
                        "No tracking angle update received in %f seconds, returning done.",
                        _updateTimeout_sec);
          
          // If no stop criteria are set, we consider this a success
          // If we have stop criteria, then this is a timeout
          const bool haveStopCriteria = HaveStopCriteria();
          if(haveStopCriteria) {
            return CheckIfDoneReturnHelper(ActionResult::TIMEOUT);
          } else {
            return CheckIfDoneReturnHelper(ActionResult::SUCCESS);
          }
        }
        else if(DEBUG_TRACKING_ACTIONS) {
          PRINT_CH_DEBUG(kLogChannelName, "ITrackAction.CheckIfDone.NotTimedOut",
                         "Current t=%f, LastUpdate t=%f, Timeout=%f",
                         currentTime, _lastUpdateTime, _updateTimeout_sec);
        }
      }
      break;
    }
      
  }
  return ActionResult::RUNNING;
}

bool ITrackAction::UpdateSmallAngleClamping()
{
  if( _clampSmallAngles ) {
    const bool hasClampPeriod = _clampSmallAnglesMaxPeriod_s > 0.0f;
    if( hasClampPeriod ) {
      const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      const bool shouldClamp = _nextTimeToClampSmallAngles_s < 0.0f || ( currTime_s >= _nextTimeToClampSmallAngles_s );
      if( shouldClamp ) {
        // re-roll the next period
        const float randPeriod_s = GetRNG().RandDblInRange(_clampSmallAnglesMinPeriod_s, _clampSmallAnglesMaxPeriod_s);
        _nextTimeToClampSmallAngles_s = currTime_s + randPeriod_s;
      }
      return shouldClamp;
    }
    else {
      // no period, so always clamp
      return true;
    }
  }
  else {
    return false;
  }
}
  
bool ITrackAction::StopCriteriaMetAndTimeToStop(const f32 relPanAngle, const f32 relTiltAngle,
                                                const f32 distance_mm, const f32 currentTime)
{
  const bool haveStopCriteria = HaveStopCriteria();
  if(haveStopCriteria)
  {
    const bool isWithinPanTol  = Util::IsFltLE(std::abs(relPanAngle), _stopCriteria.panTol.ToFloat());
    const bool isWithinTiltTol = Util::IsFltLE(std::abs(relTiltAngle), _stopCriteria.tiltTol.ToFloat());
    const bool isWithinDistTol = Util::InRange(distance_mm, _stopCriteria.minDist_mm, _stopCriteria.maxDist_mm);
    
    const bool isWithinTol = (isWithinPanTol && isWithinTiltTol && isWithinDistTol);
    
    if(DEBUG_TRACKING_ACTIONS)
    {
      PRINT_CH_DEBUG(kLogChannelName, "ITrackAction.CheckIfDone.CheckingStopCriteria",
                     "Pan:%.1fdeg vs %.1f (%c), Tilt:%.1fdeg vs %.1f (%c), Dist:%.1fmm vs (%.1f,%.1f) (%c)",
                     std::abs(RAD_TO_DEG(relPanAngle)), _stopCriteria.panTol.getDegrees(),
                     isWithinPanTol ? 'Y' : 'N',
                     std::abs(RAD_TO_DEG(relTiltAngle)), _stopCriteria.tiltTol.getDegrees(),
                     isWithinTiltTol ? 'Y' : 'N',
                     distance_mm, _stopCriteria.minDist_mm, _stopCriteria.maxDist_mm,
                     isWithinDistTol ? 'Y' : 'N');
    }
    
    if(isWithinTol)
    {
      const bool wasWithinTol = (_stopCriteria.withinTolSince_sec >= 0.f);
      
      if(wasWithinTol)
      {
        // Been within tolerance for long enough to stop yet?
        if( (currentTime - _stopCriteria.withinTolSince_sec) > _stopCriteria.duration_sec)
        {
          PRINT_CH_INFO(kLogChannelName, "ITrackAction.CheckIfDone.StopCriteriaMet",
                        "Within tolerances for > %.1fsec (panTol=%.1fdeg tiltTol=%.1fdeg distTol=[%.1f,%.1f]",
                        _stopCriteria.duration_sec,
                        _stopCriteria.panTol.getDegrees(),
                        _stopCriteria.tiltTol.getDegrees(),
                        _stopCriteria.minDist_mm, _stopCriteria.maxDist_mm);
          
          return true;
        }
      }
      else
      {
        if(DEBUG_TRACKING_ACTIONS)
        {
          PRINT_CH_DEBUG(kLogChannelName, "ITrackAction.CheckIfDone.StopCriteriaMet",
                         "Setting start of stop criteria being met to t=%.1fsec",
                         currentTime);
        }
        
        // Just got (back) into tolerance, set "since" time
        _stopCriteria.withinTolSince_sec = currentTime;
      }
    }
    else
    {
      // Not within tolerances, reset
      _stopCriteria.withinTolSince_sec = -1.f;
    }
  }
  
  return false;
}
  
//=======================================================================================================
#pragma mark -
#pragma mark TrackObjectAction

TrackObjectAction::TrackObjectAction(Robot& robot, const ObjectID& objectID, bool trackByType)
: ITrackAction(robot,
               "TrackObject",
               RobotActionType::TRACK_OBJECT)
, _objectID(objectID)
, _trackByType(trackByType)
{
  SetName("TrackObject" + std::to_string(_objectID));
}
  
TrackObjectAction::~TrackObjectAction()
{
  _robot.GetMoveComponent().UnSetTrackToObject();
}

ActionResult TrackObjectAction::InitInternal()
{
  if(!_objectID.IsSet()) {
    PRINT_NAMED_ERROR("TrackObjectAction.Init.ObjectIdNotSet", "");
    return ActionResult::BAD_OBJECT;
  }
  
  const ObservableObject* object = _robot.GetBlockWorld().GetLocatedObjectByID(_objectID);
  if(nullptr == object) {
    PRINT_NAMED_ERROR("TrackObjectAction.Init.InvalidObject",
                      "Object %d does not exist in BlockWorld", _objectID.GetValue());
    return ActionResult::BAD_OBJECT;
  }
  
  _objectType = object->GetType();
  if(_trackByType) {
    SetName("TrackObject" + std::string(EnumToString(_objectType)));
  }
  
  _lastTrackToPose = object->GetPose();
  
  _robot.GetMoveComponent().SetTrackToObject(_objectID);
  
  return ActionResult::SUCCESS;
} // InitInternal()
  
ITrackAction::UpdateResult TrackObjectAction::UpdateTracking(Radians& absPanAngle, Radians& absTiltAngle, f32& distance_mm)
{
  ObservableObject* matchingObject = nullptr;
  
  if(_trackByType) {
    BlockWorldFilter filter;
    filter.OnlyConsiderLatestUpdate(true);
    
    matchingObject = _robot.GetBlockWorld().FindLocatedClosestMatchingObject(_objectType, _lastTrackToPose, 1000.f, DEG_TO_RAD(180), filter);
    
    if(nullptr == matchingObject) {
      // Did not see an object of the right type during latest blockworld update
#     if DEBUG_TRACKING_ACTIONS
      PRINT_NAMED_INFO("TrackObjectAction.GetAngles.NoMatchingTypeFound",
                       "Could not find matching %s object.",
                       EnumToString(_objectType));
#     endif
      return UpdateResult::NoNewInfo;
    } else if(ActiveIdentityState::Identified == matchingObject->GetIdentityState()) {
      // We've possibly switched IDs that we're tracking. Keep MovementComponent's ID in sync.
      _robot.GetMoveComponent().SetTrackToObject(matchingObject->GetID());
    }
  } else {
    matchingObject = _robot.GetBlockWorld().GetLocatedObjectByID(_objectID);
    if(nullptr == matchingObject) {
      PRINT_NAMED_WARNING("TrackObjectAction.GetAngles.ObjectNoLongerExists",
                          "Object %d no longer exists in BlockWorld",
                          _objectID.GetValue());
      return UpdateResult::NoNewInfo;
    }
  }
  
  assert(nullptr != matchingObject);
  
  _lastTrackToPose = matchingObject->GetPose();
  
  // Find the observed marker closest to the robot and use that as the one we
  // track to
  std::vector<const Vision::KnownMarker*> observedMarkers;
  matchingObject->GetObservedMarkers(observedMarkers, matchingObject->GetLastObservedTime());
  
  if(observedMarkers.empty()) {
    PRINT_NAMED_ERROR("TrackObjectAction.GetAngles.NoObservedMarkers",
                      "No markers on observed object %d marked as observed since time %d, "
                      "expecting at least one.",
                      matchingObject->GetID().GetValue(),
                      matchingObject->GetLastObservedTime());
    return UpdateResult::NoNewInfo;
  }
  
  const Vision::KnownMarker* closestMarker = nullptr;
  f32 minDistSq = std::numeric_limits<f32>::max();
  f32 xDist = 0.f, yDist = 0.f, zDist = 0.f;
  
  for(auto marker : observedMarkers) {
    Pose3d markerPoseWrtRobot;
    if(false == marker->GetPose().GetWithRespectTo(_robot.GetPose(), markerPoseWrtRobot)) {
      PRINT_NAMED_ERROR("TrackObjectAction.GetAngles.PoseOriginError",
                        "Could not get pose of observed marker w.r.t. robot");
      return UpdateResult::NoNewInfo;
    }
    
    const f32 xDist_crnt = markerPoseWrtRobot.GetTranslation().x();
    const f32 yDist_crnt = markerPoseWrtRobot.GetTranslation().y();
    
    const f32 currentDistSq = xDist_crnt*xDist_crnt + yDist_crnt*yDist_crnt;
    if(currentDistSq < minDistSq) {
      closestMarker = marker;
      minDistSq = currentDistSq;
      xDist = xDist_crnt;
      yDist = yDist_crnt;
      
      // Keep track of best zDist too, so we don't have to redo the GetWithRespectTo call outside this loop
      // NOTE: This isn't perfectly accurate since it doesn't take into account the
      // the head angle and is simply using the neck joint (which should also
      // probably be queried from the robot instead of using the constant here)
      zDist = markerPoseWrtRobot.GetTranslation().z() - NECK_JOINT_POSITION[2];
    }
    
  } // For all markers
  
  if(closestMarker == nullptr) {
    PRINT_NAMED_ERROR("TrackObjectAction.GetAngles.NoClosestMarker", "");
    return UpdateResult::NoNewInfo;
  }
  
  DEV_ASSERT(minDistSq > 0.f, "Distance to closest marker should be > 0");
  
  absTiltAngle = std::atan(zDist/std::sqrt(minDistSq));
  absPanAngle  = std::atan2(yDist, xDist) + _robot.GetPose().GetRotation().GetAngleAroundZaxis();
  
  return UpdateResult::NewInfo;
  
} // UpdateTracking()
  
//=======================================================================================================
#pragma mark -
#pragma mark TrackFaceAction
  
TrackFaceAction::TrackFaceAction(Robot& robot, FaceID faceID)
  : ITrackAction(robot,
                 "TrackFace",
                 RobotActionType::TRACK_FACE)
  , _faceID(faceID)
{
   SetName("TrackFace" + std::to_string(_faceID));
}

TrackFaceAction::~TrackFaceAction()
{
  _robot.GetMoveComponent().UnSetTrackToFace();
}


ActionResult TrackFaceAction::InitInternal()
{
  if(false == _robot.HasExternalInterface()) {
    PRINT_NAMED_ERROR("TrackFaceAction.InitInternal.NoExternalInterface",
                      "Robot must have an external interface so action can "
                      "subscribe to face changed ID events.");
    return ActionResult::ABORT;
  }
  
  using namespace ExternalInterface;
  auto HandleFaceChangedID = [this](const AnkiEvent<MessageEngineToGame>& event)
  {
    auto & msg = event.GetData().Get_RobotChangedObservedFaceID();
    if(msg.oldID == _faceID)
    {
      PRINT_CH_INFO(kLogChannelName, "TrackFaceAction.HandleFaceChangedID",
                    "Updating tracked face ID from %d to %d",
                    msg.oldID, msg.newID);
      
      _faceID = msg.newID;
    }
  };
  
  _signalHandle = _robot.GetExternalInterface()->Subscribe(ExternalInterface::MessageEngineToGameTag::RobotChangedObservedFaceID, HandleFaceChangedID);
  
  _robot.GetMoveComponent().SetTrackToFace(_faceID);
  _lastFaceUpdate = 0;
  
  return ActionResult::SUCCESS;
} // InitInternal()

void TrackFaceAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
{
  TrackFaceCompleted completion;
  completion.faceID = static_cast<s32>(_faceID);
  completionUnion.Set_trackFaceCompleted(std::move(completion));
}
  
ITrackAction::UpdateResult TrackFaceAction::UpdateTracking(Radians& absPanAngle, Radians& absTiltAngle, f32& distance_mm)
{
  const Vision::TrackedFace* face = _robot.GetFaceWorld().GetFace(_faceID);
  distance_mm = 0.f;
  
  if(nullptr == face) {
    // No such face
    PRINT_CH_INFO(kLogChannelName, "TrackFaceAction.GetAngles.BadFaceID", "No face %d in FaceWorld", _faceID);
    return UpdateResult::NoNewInfo;
  }
  
  // Only update pose if we've actually observed the face again since last update
  if(face->GetTimeStamp() <= _lastFaceUpdate) {
    return UpdateResult::NoNewInfo;
  }
  _lastFaceUpdate = face->GetTimeStamp();
  
  Pose3d headPoseWrtRobot;
  if(false == face->GetHeadPose().GetWithRespectTo(_robot.GetPose(), headPoseWrtRobot)) {
    PRINT_NAMED_ERROR("TrackFaceAction.GetAngles.PoseOriginError",
                      "Could not get pose of face w.r.t. robot.");
    return UpdateResult::NoNewInfo;
  }
  
  const f32 xDist = headPoseWrtRobot.GetTranslation().x();
  const f32 yDist = headPoseWrtRobot.GetTranslation().y();
  
  // NOTE: This isn't perfectly accurate since it doesn't take into account the
  // the head angle and is simply using the neck joint (which should also
  // probably be queried from the robot instead of using the constant here)
  const f32 zDist = headPoseWrtRobot.GetTranslation().z() - NECK_JOINT_POSITION[2];

# if DEBUG_TRACKING_ACTIONS
  PRINT_NAMED_INFO("TrackFaceAction.GetAngles.HeadPose",
                   "Translation w.r.t. robot = (%.1f, %.1f, %.1f) [t=%d]",
                   xDist, yDist, zDist, face->GetTimeStamp());
# endif
  
  const f32 xyDistSq = xDist*xDist + yDist*yDist;
  if (xyDistSq <= 0.f)
  {
    DEV_ASSERT(false, "TrackFaceAction.GetAngles.ZeroDistance");
    return UpdateResult::NoNewInfo;
  }
  
  absTiltAngle = std::atan(zDist/std::sqrt(xyDistSq));
  absPanAngle  = std::atan2(yDist, xDist) + _robot.GetPose().GetRotation().GetAngleAroundZaxis();

  return UpdateResult::NewInfo;
} // UpdateTracking()
  
//=======================================================================================================
#pragma mark -
#pragma mark TrackPetFaceAction
  
TrackPetFaceAction::TrackPetFaceAction(Robot& robot, FaceID faceID)
: ITrackAction(robot,
               "TrackPetFace" + std::to_string(faceID),
               RobotActionType::TRACK_PET_FACE)
, _faceID(faceID)
{

}

TrackPetFaceAction::TrackPetFaceAction(Robot& robot, Vision::PetType petType)
: ITrackAction(robot,
               "TrackPetFace",
               RobotActionType::TRACK_PET_FACE)
, _petType(petType)
{
  switch(_petType)
  {
    case Vision::PetType::Cat:
      SetName("TrackCatFace");
      break;
   
    case Vision::PetType::Dog:
      SetName("TrackDogFace");
      break;
      
    case Vision::PetType::Unknown:
      SetName("TrackAnyPetFace");
      break;
  }
}

ActionResult TrackPetFaceAction::InitInternal()
{
  _lastFaceUpdate = 0;
  
  return ActionResult::SUCCESS;
} // InitInternal()

void TrackPetFaceAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
{
  TrackFaceCompleted completion;
  completion.faceID = static_cast<s32>(_faceID);
  completionUnion.Set_trackFaceCompleted(std::move(completion));
}

ITrackAction::UpdateResult TrackPetFaceAction::UpdateTracking(Radians& absPanAngle, Radians& absTiltAngle, f32& distance_mm)
{
  const Vision::TrackedPet* petFace = nullptr;
  
  if(_faceID != Vision::UnknownFaceID)
  {
    petFace = _robot.GetPetWorld().GetPetByID(_faceID);
    
    if(nullptr == petFace)
    {
      // No such face
      PRINT_CH_INFO(kLogChannelName, "TrackPetFaceAction.GetAngles.BadFaceID", "No face %d in PetWorld", _faceID);
      return UpdateResult::NoNewInfo;
    }
  }
  else
  {
    auto petIDs = _robot.GetPetWorld().GetKnownPetsWithType(_petType);
    if(!petIDs.empty())
    {
      petFace = _robot.GetPetWorld().GetPetByID(*petIDs.begin());
    }
    else
    {
      PRINT_CH_INFO(kLogChannelName, "TrackPetFaceAction.GetAngles.NoPetsWithType", "Type=%s",
                    EnumToString(_petType));
      return UpdateResult::NoNewInfo;
    }
  }

  // Only update pose if we've actually observed the face again since last update
  if(petFace->GetTimeStamp() <= _lastFaceUpdate) {
    return UpdateResult::NoNewInfo;
  }
  _lastFaceUpdate = petFace->GetTimeStamp();
  
  Result result = _robot.ComputeTurnTowardsImagePointAngles(petFace->GetRect().GetMidPoint(), petFace->GetTimeStamp(),
                                                            absPanAngle, absTiltAngle);
  if(RESULT_OK != result)
  {
    PRINT_NAMED_WARNING("TrackpetFaceAction.GetAngles.ComputeTurnTowardsImagePointAnglesFailed", "t=%u", petFace->GetTimeStamp());
    return UpdateResult::NoNewInfo;
  }
  
  return UpdateResult::NewInfo;
} // UpdateTracking()
  
//=======================================================================================================
#pragma mark -
#pragma mark TrackMotionAction
  
ActionResult TrackMotionAction::InitInternal()
{
  if(false == _robot.HasExternalInterface()) {
    PRINT_NAMED_ERROR("TrackMotionAction.Init.NoExternalInterface",
                      "Robot must have an external interface so action can "
                      "subscribe to motion observation events.");
    return ActionResult::ABORT;
  }
  
  _gotNewMotionObservation = false;
  
  using namespace ExternalInterface;
  auto HandleObservedMotion = [this](const AnkiEvent<MessageEngineToGame>& event)
  {
    _gotNewMotionObservation = true;
    this->_motionObservation = event.GetData().Get_RobotObservedMotion();
  };
  
  _signalHandle = _robot.GetExternalInterface()->Subscribe(ExternalInterface::MessageEngineToGameTag::RobotObservedMotion, HandleObservedMotion);
  
  return ActionResult::SUCCESS;
} // InitInternal()

  
ITrackAction::UpdateResult TrackMotionAction::UpdateTracking(Radians& absPanAngle, Radians& absTiltAngle, f32& distance_mm)
{
  distance_mm = 0.f;
  
  if(_gotNewMotionObservation && _motionObservation.img_area > 0)
  {
    _gotNewMotionObservation = false;
    
    const Point2f motionCentroid(_motionObservation.img_x, _motionObservation.img_y);
    
    // Note: we start with relative angles here, but make them absolute below.
    _robot.GetVisionComponent().GetCamera().ComputePanAndTiltAngles(motionCentroid, absPanAngle, absTiltAngle);
    
    // Find pose of robot at time motion was observed
    HistRobotState* histStatePtr = nullptr;
    TimeStamp_t junkTime;
    if(RESULT_OK != _robot.GetStateHistory()->ComputeAndInsertStateAt(_motionObservation.timestamp, junkTime, &histStatePtr)) {
      PRINT_NAMED_ERROR("TrackMotionAction.GetAngles.PoseHistoryError",
                        "Could not get historical pose for motion observed at t=%d (lastRobotMsgTime = %d)",
                        _motionObservation.timestamp,
                        _robot.GetLastMsgTimestamp());
      return UpdateResult::NoNewInfo;
    }
    
    assert(nullptr != histStatePtr);
    
    // Make absolute
    absTiltAngle += histStatePtr->GetHeadAngle_rad();
    absPanAngle  += histStatePtr->GetPose().GetRotation().GetAngleAroundZaxis();
    
#   if DEBUG_TRACKING_ACTIONS
    PRINT_NAMED_INFO("TrackMotionAction.GetAngles.Motion",
                     "Motion area=%.1f%%, centroid=(%.1f,%.1f)",
                     _motionObservation.img_area * 100.f,
                     motionCentroid.x(), motionCentroid.y());
#   endif
    
    return UpdateResult::NewInfo;
    
  } // if(_gotNewMotionObservation && _motionObservation.img_area > 0)
  
  return UpdateResult::NoNewInfo;
  
} // UpdateTracking()
  
} // namespace Cozmo
} // namespace Anki
