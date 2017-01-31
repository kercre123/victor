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

void ITrackAction::SetTiltSpeeds(f32 minSpeed_radPerSec, f32 maxSpeed_radPerSec) {
  if(minSpeed_radPerSec >= maxSpeed_radPerSec) {
    PRINT_NAMED_WARNING("ITrackAction.SetTiltSpeeds.InvalidSpeeds",
                        "Min (%f) should be < max (%f)",
                        minSpeed_radPerSec, maxSpeed_radPerSec);
  } else {
    _minTiltSpeed_radPerSec = minSpeed_radPerSec;
    _maxTiltSpeed_radPerSec = maxSpeed_radPerSec;
  }
}

void ITrackAction::SetPanSpeeds(f32 minSpeed_radPerSec,  f32 maxSpeed_radPerSec) {
  if(minSpeed_radPerSec >= maxSpeed_radPerSec) {
    PRINT_NAMED_WARNING("ITrackAction.SetPanSpeeds.InvalidSpeeds",
                        "Min (%f) should be < max (%f)",
                        minSpeed_radPerSec, maxSpeed_radPerSec);
  } else {
    _minPanSpeed_radPerSec = minSpeed_radPerSec;
    _maxPanSpeed_radPerSec = maxSpeed_radPerSec;
  }
}

void ITrackAction::SetMode(Mode newMode)
{
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
  
  return InitInternal();
}
  
bool ITrackAction::InterruptInternal()
{
  _lastUpdateTime = 0.f;
  return true;
}

ActionResult ITrackAction::CheckIfDone()
{

  if(_stopOnOtherActionTag != ActionConstants::INVALID_TAG &&
     !IsTagInUse(_stopOnOtherActionTag) )
  {
    PRINT_CH_INFO(kLogChannelName, "ITrackAction.FinishedByOtherAction",
                  "[%d] action %s stopping because we were told to stop when another action stops (and it did)",
                  GetTag(),
                  GetName().c_str());
    return ActionResult::SUCCESS;
  }
  
  Radians absPanAngle = 0, absTiltAngle = 0;

  const double currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  // See if there are new absolute pan/tilt angles from the derived class
  if(GetAngles(absPanAngle, absTiltAngle))
  {

    if( absTiltAngle > _maxHeadAngle ) {
      absTiltAngle = _maxHeadAngle;
    }
    
    // Record latest update to avoid timing out
    if(_updateTimeout_sec > 0.) {
      _lastUpdateTime = currentTime;
    }
    
#   if DEBUG_TRACKING_ACTIONS
    PRINT_NAMED_INFO("ITrackAction.CheckIfDone.NewAngles",
                     "Commanding abs angles: pan=%.1fdeg, tilt=%.1fdeg",
                     absPanAngle.getDegrees(), absTiltAngle.getDegrees());
#   endif
    
    bool angleLargeEnoughForSound = false;
    f32  eyeShiftX = 0.f, eyeShiftY = 0.f;
    
    // Tilt Head:
    f32 relTiltAngle = (absTiltAngle - _robot.GetHeadAngle()).ToFloat();
    
    // If enabled, always move at least the tolerance amount
    if(_clampSmallAngles && FLT_LE(std::abs(relTiltAngle), _tiltTolerance.ToFloat()))
    {
      relTiltAngle = std::copysign(_tiltTolerance.ToFloat(), relTiltAngle);
      absTiltAngle = _robot.GetHeadAngle() + relTiltAngle;
    }
    
    if((Mode::HeadAndBody == _mode || Mode::HeadOnly == _mode) &&
       FLT_GE(std::abs(relTiltAngle), _tiltTolerance.ToFloat()))
    {
      // Set speed/accel based on angle difference
      const f32 angleFrac = std::abs(relTiltAngle)/(MAX_HEAD_ANGLE-MIN_HEAD_ANGLE);
      const f32 speed = (_maxTiltSpeed_radPerSec - _minTiltSpeed_radPerSec)*angleFrac + _minTiltSpeed_radPerSec;
      const f32 accel = 20.f; // (MaxAccel - MinAccel)*angleFrac + MinAccel;
      
      if(RESULT_OK != _robot.GetMoveComponent().MoveHeadToAngle(absTiltAngle.ToFloat(), speed, accel))
      {
        return ActionResult::SEND_MESSAGE_TO_ROBOT_FAILED;
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
    
    // If enabled, always move at least the tolerance amount
    if(_clampSmallAngles && FLT_LE(std::abs(relPanAngle), _panTolerance.ToFloat()))
    {
      relPanAngle = std::copysign(_panTolerance.ToFloat(), relPanAngle);
      absPanAngle = _robot.GetPose().GetRotation().GetAngleAroundZaxis().ToFloat() + relPanAngle;
    }
    
    if((Mode::HeadAndBody == _mode || Mode::BodyOnly == _mode) &&
       FLT_GE(std::abs(relPanAngle), _panTolerance.ToFloat()))
    {
      // Get rotation angle around drive center
      Pose3d rotatedPose;
      Pose3d dcPose = _robot.GetDriveCenterPose();
      dcPose.SetRotation(absPanAngle, Z_AXIS_3D());
      _robot.ComputeOriginPose(dcPose, rotatedPose);
      
      // Set speed/accel based on angle difference
      const f32 angleFrac = std::min(1.f, std::abs(relPanAngle)/M_PI_F);
      const f32 speed = (_maxPanSpeed_radPerSec - _minPanSpeed_radPerSec)*angleFrac + _minPanSpeed_radPerSec;
      const f32 accel = 10.f; //(MaxAccel - MinAccel)*angleFrac + MinAccel;
      
      RobotInterface::SetBodyAngle setBodyAngle;
      setBodyAngle.angle_rad             = rotatedPose.GetRotation().GetAngleAroundZaxis().ToFloat();
      setBodyAngle.max_speed_rad_per_sec = speed;
      setBodyAngle.accel_rad_per_sec2    = accel;
      setBodyAngle.angle_tolerance       = _panTolerance.ToFloat();
      if(RESULT_OK != _robot.SendRobotMessage<RobotInterface::SetBodyAngle>(std::move(setBodyAngle))) {
        return ActionResult::SEND_MESSAGE_TO_ROBOT_FAILED;
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
    const bool haveTurningSoundAnim = AnimationTrigger::Count != _turningSoundAnimTrigger;
    if(haveTurningSoundAnim && currentTime > _nextSoundTime && angleLargeEnoughForSound)
    {
      // Queue sound to only play if nothing else is playing
      PlayAnimationAction* soundAction = new TriggerLiftSafeAnimationAction(_robot, _turningSoundAnimTrigger, 1, false);
      _soundAnimTag = soundAction->GetTag();
      _robot.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, soundAction);
      
      _nextSoundTime = currentTime + GetRNG().RandDblInRange(_soundSpacingMin_sec, _soundSpacingMax_sec);
    }
    
    // Move eyes if indicated
    if(_moveEyes && (eyeShiftX != 0.f || eyeShiftY != 0.f))
    {
      // Clip, but retain sign
      eyeShiftX = CLIP(eyeShiftX, (f32)-ProceduralFace::WIDTH/4,  (f32)ProceduralFace::WIDTH/4);
      eyeShiftY = CLIP(eyeShiftY, (f32)-ProceduralFace::HEIGHT/4, (f32)ProceduralFace::HEIGHT/4);
      
#     if DEBUG_TRACKING_ACTIONS
      PRINT_NAMED_DEBUG("ITrackAction.CheckIfDone.EyeShift",
                        "Adjusting eye shift to (%.1f,%.1f), with tag=%d",
                        eyeShiftX, eyeShiftY, _eyeShiftTag);
#     endif
      
      // Expose as params?
      const f32 maxLookUpScale   = 1.1f;
      const f32 minLookDownScale = 0.8f;
      const f32 outerEyeScaleIncrease = 0.1f;
      
      ProceduralFace procFace;
      procFace.LookAt(eyeShiftX, eyeShiftY,
                      static_cast<f32>(ProceduralFace::WIDTH/4),
                      static_cast<f32>(ProceduralFace::HEIGHT/4),
                      maxLookUpScale, minLookDownScale, outerEyeScaleIncrease);
      
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
    
  } else if(_updateTimeout_sec > 0.) {
    if(currentTime - _lastUpdateTime > _updateTimeout_sec) {
      PRINT_CH_INFO(kLogChannelName, "ITrackAction.CheckIfDone.Timeout",
                    "No tracking angle update received in %f seconds, returning done.",
                    _updateTimeout_sec);
      return ActionResult::SUCCESS;
    }
    
  }
  
  return ActionResult::RUNNING;
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
  
bool TrackObjectAction::GetAngles(Radians& absPanAngle, Radians& absTiltAngle)
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
      return false;
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
      return false;
    }
  }
  
  assert(nullptr != matchingObject);
  
  DEV_ASSERT(PoseState::Unknown != matchingObject->GetPoseState(), "Object's pose state should not be Unknown");
  
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
    return false;
  }
  
  const Vision::KnownMarker* closestMarker = nullptr;
  f32 minDistSq = std::numeric_limits<f32>::max();
  f32 xDist = 0.f, yDist = 0.f, zDist = 0.f;
  
  for(auto marker : observedMarkers) {
    Pose3d markerPoseWrtRobot;
    if(false == marker->GetPose().GetWithRespectTo(_robot.GetPose(), markerPoseWrtRobot)) {
      PRINT_NAMED_ERROR("TrackObjectAction.GetAngles.PoseOriginError",
                        "Could not get pose of observed marker w.r.t. robot");
      return false;
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
    return false;
  }
  
  DEV_ASSERT(minDistSq > 0.f, "Distance to closest marker should be > 0");
  
  absTiltAngle = std::atan(zDist/std::sqrt(minDistSq));
  absPanAngle  = std::atan2(yDist, xDist) + _robot.GetPose().GetRotation().GetAngleAroundZaxis();
  
  return true;
  
} // GetAngles()
  
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
  
bool TrackFaceAction::GetAngles(Radians& absPanAngle, Radians& absTiltAngle)
{
  const Vision::TrackedFace* face = _robot.GetFaceWorld().GetFace(_faceID);
  
  if(nullptr == face) {
    // No such face
    PRINT_CH_INFO(kLogChannelName, "TrackFaceAction.GetAngles.BadFaceID", "No face %d in FaceWorld", _faceID);
    return false;
  }
  
  // Only update pose if we've actually observed the face again since last update
  if(face->GetTimeStamp() <= _lastFaceUpdate) {
    return false;
  }
  _lastFaceUpdate = face->GetTimeStamp();
  
  Pose3d headPoseWrtRobot;
  if(false == face->GetHeadPose().GetWithRespectTo(_robot.GetPose(), headPoseWrtRobot)) {
    PRINT_NAMED_ERROR("TrackFaceAction.GetAngles.PoseOriginError",
                      "Could not get pose of face w.r.t. robot.");
    return false;
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
    return false;
  }
  
  absTiltAngle = std::atan(zDist/std::sqrt(xyDistSq));
  absPanAngle  = std::atan2(yDist, xDist) + _robot.GetPose().GetRotation().GetAngleAroundZaxis();

  return true;
} // GetAngles()
  
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

bool TrackPetFaceAction::GetAngles(Radians& absPanAngle, Radians& absTiltAngle)
{
  const Vision::TrackedPet* petFace = nullptr;
  
  if(_faceID != Vision::UnknownFaceID)
  {
    petFace = _robot.GetPetWorld().GetPetByID(_faceID);
    
    if(nullptr == petFace)
    {
      // No such face
      PRINT_CH_INFO(kLogChannelName, "TrackPetFaceAction.GetAngles.BadFaceID", "No face %d in PetWorld", _faceID);
      return false;
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
      return false;
    }
  }

  // Only update pose if we've actually observed the face again since last update
  if(petFace->GetTimeStamp() <= _lastFaceUpdate) {
    return false;
  }
  _lastFaceUpdate = petFace->GetTimeStamp();
  
  Result result = _robot.ComputeTurnTowardsImagePointAngles(petFace->GetRect().GetMidPoint(), petFace->GetTimeStamp(),
                                                            absPanAngle, absTiltAngle);
  if(RESULT_OK != result)
  {
    PRINT_NAMED_WARNING("TrackpetFaceAction.GetAngles.ComputeTurnTowardsImagePointAnglesFailed", "t=%u", petFace->GetTimeStamp());
    return false;
  }
  
  return true;
} // GetAngles()
  
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

  
bool TrackMotionAction::GetAngles(Radians& absPanAngle, Radians& absTiltAngle)
{
  if(_gotNewMotionObservation && _motionObservation.img_area > 0)
  {
    _gotNewMotionObservation = false;
    
    const Point2f motionCentroid(_motionObservation.img_x, _motionObservation.img_y);
    
    // Note: we start with relative angles here, but make them absolute below.
    _robot.GetVisionComponent().GetCamera().ComputePanAndTiltAngles(motionCentroid, absPanAngle, absTiltAngle);
    
    // Find pose of robot at time motion was observed
    RobotPoseStamp* poseStamp = nullptr;
    TimeStamp_t junkTime;
    if(RESULT_OK != _robot.GetPoseHistory()->ComputeAndInsertPoseAt(_motionObservation.timestamp, junkTime, &poseStamp)) {
      PRINT_NAMED_ERROR("TrackMotionAction.GetAngles.PoseHistoryError",
                        "Could not get historical pose for motion observed at t=%d (lastRobotMsgTime = %d)",
                        _motionObservation.timestamp,
                        _robot.GetLastMsgTimestamp());
      return false;
    }
    
    assert(nullptr != poseStamp);
    
    // Make absolute
    absTiltAngle += poseStamp->GetHeadAngle();
    absPanAngle  += poseStamp->GetPose().GetRotation().GetAngleAroundZaxis();
    
#   if DEBUG_TRACKING_ACTIONS
    PRINT_NAMED_INFO("TrackMotionAction.GetAngles.Motion",
                     "Motion area=%.1f%%, centroid=(%.1f,%.1f)",
                     _motionObservation.img_area * 100.f,
                     motionCentroid.x(), motionCentroid.y());
#   endif
    
    return true;
    
  } // if(_gotNewMotionObservation && _motionObservation.img_area > 0)
  
  return false;
  
} // GetAngles()
  
  
} // namespace Cozmo
} // namespace Anki
