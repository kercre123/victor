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


#include "anki/cozmo/basestation/actions/trackingActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"

#include "clad/externalInterface/messageEngineToGameTag.h"
#include "clad/externalInterface/messageEngineToGame.h"

#include "anki/common/basestation/utils/timer.h"

#define DEBUG_TRACKING_ACTIONS 0

namespace Anki {
namespace Cozmo {
  
#pragma mark -
#pragma mark ITrackAction
  
ITrackAction::ITrackAction(Robot& robot, const std::string name, const RobotActionType type)
: IAction(robot,
          name,
          type,
          ((u8)AnimTrackFlag::BODY_TRACK | (u8)AnimTrackFlag::HEAD_TRACK))
, _eyeShiftTag(AnimationStreamer::NotAnimatingTag)
{
  
}

ITrackAction::~ITrackAction()
{
  if(_eyeShiftTag != AnimationStreamer::NotAnimatingTag) {
    // Make sure any eye shift gets removed
    _robot.GetAnimationStreamer().RemovePersistentFaceLayer(_eyeShiftTag);
    _eyeShiftTag = AnimationStreamer::NotAnimatingTag;
  }

  // Make sure to restore original eye dart distance
  _robot.GetAnimationStreamer().SetParam(LiveIdleAnimationParameter::EyeDartMaxDistance_pix, _originalEyeDartDist);
  
  // Make sure we abort any sound actions we triggered
  _robot.GetActionList().Cancel(_soundAnimTag);
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
  if(GetState() == ActionResult::FAILURE_NOT_STARTED)
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
}
  
void ITrackAction::SetPanTolerance(const Radians& panThreshold)
{
  _panTolerance = panThreshold.getAbsoluteVal();
  
  // NOTE: can't be lower than what is used internally on the robot
  if( _panTolerance.ToFloat() < POINT_TURN_ANGLE_TOL ) {
    PRINT_NAMED_WARNING("ITrackAction.InvalidTolerance",
                        "Tried to set tolerance of %fdeg, min is %f",
                        RAD_TO_DEG(_panTolerance.ToFloat()),
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
                        RAD_TO_DEG(_tiltTolerance.ToFloat()),
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
  
  return InitInternal();
}
  
bool ITrackAction::InterruptInternal()
{
  _lastUpdateTime = 0.f;
  return true;
}

ActionResult ITrackAction::CheckIfDone()
{
  Radians absPanAngle = 0, absTiltAngle = 0;
  
  const double currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  // See if there are new relative pan/tilt angles from the derived class
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
    const f32 relTiltAngle = (absTiltAngle - _robot.GetHeadAngle()).ToFloat();
    if((Mode::HeadAndBody == _mode || Mode::HeadOnly == _mode) &&
       std::abs(relTiltAngle) > _tiltTolerance.ToFloat())
    {
      // Set speed/accel based on angle difference
      const f32 angleFrac = std::abs(relTiltAngle)/(MAX_HEAD_ANGLE-MIN_HEAD_ANGLE);
      const f32 speed = (_maxTiltSpeed_radPerSec - _minTiltSpeed_radPerSec)*angleFrac + _minTiltSpeed_radPerSec;
      const f32 accel = 20.f; // (MaxAccel - MinAccel)*angleFrac + MinAccel;
      
      if(RESULT_OK != _robot.GetMoveComponent().MoveHeadToAngle(absTiltAngle.ToFloat(), speed, accel))
      {
        return ActionResult::FAILURE_ABORT;
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
    const f32 relPanAngle = (absPanAngle - _robot.GetPose().GetRotation().GetAngleAroundZaxis()).ToFloat();
    if((Mode::HeadAndBody == _mode || Mode::BodyOnly == _mode) && std::abs(relPanAngle) > _panTolerance.ToFloat())
    {
      // Get rotation angle around drive center
      Pose3d rotatedPose;
      Pose3d dcPose = _robot.GetDriveCenterPose();
      dcPose.SetRotation(absPanAngle, Z_AXIS_3D());
      _robot.ComputeOriginPose(dcPose, rotatedPose);
      
      // Set speed/accel based on angle difference
      const f32 angleFrac = std::min(1.f, std::abs(relPanAngle)/(f32)M_PI);
      const f32 speed = (_maxPanSpeed_radPerSec - _minPanSpeed_radPerSec)*angleFrac + _minPanSpeed_radPerSec;
      const f32 accel = 10.f; //(MaxAccel - MinAccel)*angleFrac + MinAccel;
      
      RobotInterface::SetBodyAngle setBodyAngle;
      setBodyAngle.angle_rad             = rotatedPose.GetRotation().GetAngleAroundZaxis().ToFloat();
      setBodyAngle.max_speed_rad_per_sec = speed;
      setBodyAngle.accel_rad_per_sec2    = accel;
      setBodyAngle.angle_tolerance       = _panTolerance.ToFloat();
      if(RESULT_OK != _robot.SendRobotMessage<RobotInterface::SetBodyAngle>(std::move(setBodyAngle))) {
        return ActionResult::FAILURE_ABORT;
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
    if( currentTime > _nextSoundTime && angleLargeEnoughForSound)
    {
      // Queue sound to only play if nothing else is playing
      PlayAnimationAction* soundAction = new TriggerAnimationAction(_robot, _turningSoundAnimTrigger, 1, false);
      _soundAnimTag = soundAction->GetTag();
      _robot.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, soundAction);
      
      _nextSoundTime = currentTime + GetRNG().RandDblInRange(_soundSpacingMin_sec, _soundSpacingMax_sec);
    }
    
    // Move eyes if indicated
    if(_moveEyes && (eyeShiftX != 0.f || eyeShiftY != 0.f))
    {
      // Clip, but retain sign
      eyeShiftX = CLIP(eyeShiftX, -ProceduralFace::WIDTH/4, ProceduralFace::WIDTH/4);
      eyeShiftY = CLIP(eyeShiftY, -ProceduralFace::HEIGHT/4, ProceduralFace::HEIGHT/4);
      
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
      PRINT_NAMED_INFO("ITrackAction.CheckIfDone.Timeout",
                       "No tracking angle update received in %f seconds, returning done.",
                       _updateTimeout_sec);
      return ActionResult::SUCCESS;
    }
    
  }
  
  return ActionResult::RUNNING;
}
  
#pragma mark -
#pragma mark TrackObjectAction

TrackObjectAction::TrackObjectAction(Robot& robot, const ObjectID& objectID, bool trackByType)
: ITrackAction(robot,
               "TrackObject",
               RobotActionType::TRACK_OBJECT)
, _objectID(objectID)
, _trackByType(trackByType)
{

}
  
TrackObjectAction::~TrackObjectAction()
{
  _robot.GetMoveComponent().UnSetTrackToObject();
}

ActionResult TrackObjectAction::InitInternal()
{
  if(!_objectID.IsSet()) {
    PRINT_NAMED_ERROR("TrackObjectAction.Init.ObjectIdNotSet", "");
    return ActionResult::FAILURE_ABORT;
  }
  
  const ObservableObject* object = _robot.GetBlockWorld().GetObjectByID(_objectID);
  if(nullptr == object) {
    PRINT_NAMED_ERROR("TrackObjectAction.Init.InvalidObject",
                      "Object %d does not exist in BlockWorld", _objectID.GetValue());
    return ActionResult::FAILURE_ABORT;
  }
  
  _objectType = object->GetType();
  _lastTrackToPose = object->GetPose();
  
  if(_trackByType) {
    SetName("TrackObject" + std::string(EnumToString(_objectType)));
  } else {
    SetName("TrackObject" + std::to_string(_objectID));
  }
  
  _robot.GetMoveComponent().SetTrackToObject(_objectID);
  
  return ActionResult::SUCCESS;
} // InitInternal()
  
bool TrackObjectAction::GetAngles(Radians& absPanAngle, Radians& absTiltAngle)
{
  ObservableObject* matchingObject = nullptr;
  
  if(_trackByType) {
    BlockWorldFilter filter;
    filter.OnlyConsiderLatestUpdate(true);
    
    matchingObject = _robot.GetBlockWorld().FindClosestMatchingObject(_objectType, _lastTrackToPose, 1000.f, DEG_TO_RAD(180), filter);
    
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
    matchingObject = _robot.GetBlockWorld().GetObjectByID(_objectID);
    if(nullptr == matchingObject) {
      PRINT_NAMED_WARNING("TrackObjectAction.GetAngles.ObjectNoLongerExists",
                          "Object %d no longer exists in BlockWorld",
                          _objectID.GetValue());
      return false;
    }
  }
  
  assert(nullptr != matchingObject);
  
  ASSERT_NAMED(PoseState::Unknown != matchingObject->GetPoseState(),
               "Object's pose state should not be Unknkown");
  
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
  
  ASSERT_NAMED(minDistSq > 0.f, "Distance to closest marker should be > 0.");
  
  absTiltAngle = std::atan(zDist/std::sqrt(minDistSq));
  absPanAngle  = std::atan2(yDist, xDist) + _robot.GetPose().GetRotation().GetAngleAroundZaxis();
  
  return true;
  
} // GetAngles()
  
  
#pragma mark -
#pragma mark TrackFaceAction
  
TrackFaceAction::TrackFaceAction(Robot& robot, FaceID faceID)
  : ITrackAction(robot,
                 "TrackFace",
                 RobotActionType::TRACK_FACE)
  , _faceID(faceID)
{
  
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
    return ActionResult::FAILURE_ABORT;
  }
  
  using namespace ExternalInterface;
  auto HandleFaceChangedID = [this](const AnkiEvent<MessageEngineToGame>& event)
  {
    auto & msg = event.GetData().Get_RobotChangedObservedFaceID();
    if(msg.oldID == _faceID)
    {
      PRINT_NAMED_INFO("TrackFaceAction.HandleFaceChangedID",
                       "Updating tracked face ID from %d to %d",
                       msg.oldID, msg.newID);
      
      _faceID = msg.newID;
    }
  };
  
  _signalHandle = _robot.GetExternalInterface()->Subscribe(ExternalInterface::MessageEngineToGameTag::RobotChangedObservedFaceID, HandleFaceChangedID);
  
  SetName("TrackFace" + std::to_string(_faceID));
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
    PRINT_NAMED_INFO("TrackFaceAction.GetAngles.BadFaceID", "No face %d in FaceWorld", _faceID);
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
    ASSERT_NAMED(false, "TrackFaceAction.GetAngles.ZeroDistance");
    return false;
  }
  
  absTiltAngle = std::atan(zDist/std::sqrt(xyDistSq));
  absPanAngle  = std::atan2(yDist, xDist) + _robot.GetPose().GetRotation().GetAngleAroundZaxis();

  return true;
} // GetAngles()
  
  
#pragma mark -
#pragma mark TrackMotionAction
  
ActionResult TrackMotionAction::InitInternal()
{
  if(false == _robot.HasExternalInterface()) {
    PRINT_NAMED_ERROR("TrackMotionAction.Init.NoExternalInterface",
                      "Robot must have an external interface so action can "
                      "subscribe to motion observation events.");
    return ActionResult::FAILURE_ABORT;
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
