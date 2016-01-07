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


#include "anki/cozmo/basestation/trackingActions.h"
#include "anki/cozmo/basestation/cozmoActions.h"
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
  
  
u8 ITrackAction::GetAnimTracksToDisable() const
{
  switch(_mode)
  {
    case Mode::HeadAndBody:
      return (u8)AnimTrackFlag::BODY_TRACK | (u8)AnimTrackFlag::HEAD_TRACK;
      
    case Mode::HeadOnly:
      return (u8)AnimTrackFlag::HEAD_TRACK;
      
    case Mode::BodyOnly:
      return (u8)AnimTrackFlag::BODY_TRACK;
  }
}
  
u8 ITrackAction::GetMovementTracksToIgnore() const
{
  return GetAnimTracksToDisable();
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
  
bool ITrackAction::InterruptInternal()
{
  _lastUpdateTime = 0.f;
  return true;
}

ActionResult ITrackAction::CheckIfDone(Robot& robot)
{
  Radians absPanAngle = 0, absTiltAngle = 0;
  
  const double currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  // See if there are new relative pan/tilt angles from the derived class
  if(GetAngles(robot, absPanAngle, absTiltAngle))
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
    
    // Tilt Head:
    const f32 relTiltAngle = std::abs((absTiltAngle - robot.GetHeadAngle()).ToFloat());
    if((Mode::HeadAndBody == _mode || Mode::HeadOnly == _mode) && relTiltAngle > _tiltTolerance.ToFloat())
    {
      // Set speed/accel based on angle difference
      const f32 MinSpeed = 30.f;
      const f32 MaxSpeed = 50.f;
      //const f32 MinAccel = 20.f;
      //const f32 MaxAccel = 40.f;
      
      const f32 angleFrac = relTiltAngle/(MAX_HEAD_ANGLE-MIN_HEAD_ANGLE);
      const f32 speed = (MaxSpeed - MinSpeed)*angleFrac + MinSpeed;
      const f32 accel = 20.f; // (MaxAccel - MinAccel)*angleFrac + MinAccel;
      
      if(RESULT_OK != robot.GetMoveComponent().MoveHeadToAngle(absTiltAngle.ToFloat(), speed, accel))
      {
        return ActionResult::FAILURE_ABORT;
      }
      
      if(relTiltAngle > _minTiltAngleForSound) {
        angleLargeEnoughForSound = true;
      }
    }
    
    // Pan Body:
    const f32 relPanAngle = std::abs((absPanAngle - robot.GetPose().GetRotation().GetAngleAroundZaxis()).ToFloat());
    if((Mode::HeadAndBody == _mode || Mode::BodyOnly == _mode) && relPanAngle > _panTolerance.ToFloat())
    {
      // Get rotation angle around drive center
      Pose3d rotatedPose;
      Pose3d dcPose = robot.GetDriveCenterPose();
      dcPose.SetRotation(absPanAngle, Z_AXIS_3D());
      robot.ComputeOriginPose(dcPose, rotatedPose);
      
      // Set speed/accel based on angle difference
      const f32 MinSpeed = 20.f;
      const f32 MaxSpeed = 80.f;
      //const f32 MinAccel = 30.f;
      //const f32 MaxAccel = 80.f;
      
      const f32 angleFrac = std::min(1.f, relPanAngle/(f32)M_PI);
      const f32 speed = (MaxSpeed - MinSpeed)*angleFrac + MinSpeed;
      const f32 accel = 10.f; //(MaxAccel - MinAccel)*angleFrac + MinAccel;
      
      RobotInterface::SetBodyAngle setBodyAngle;
      setBodyAngle.angle_rad             = rotatedPose.GetRotation().GetAngleAroundZaxis().ToFloat();
      setBodyAngle.max_speed_rad_per_sec = speed;
      setBodyAngle.accel_rad_per_sec2    = accel; 
      if(RESULT_OK != robot.SendRobotMessage<RobotInterface::SetBodyAngle>(std::move(setBodyAngle))) {
        return ActionResult::FAILURE_ABORT;
      }
      
      if(relPanAngle > _minPanAngleForSound) {
        angleLargeEnoughForSound = true;
      }
    }
    
    // Play sound if it's time and either angle was big enough
    if(!_turningSoundAnimation.empty() && currentTime > _nextSoundTime && angleLargeEnoughForSound)
    {
      // Queue sound to only play if nothing else is playing
      robot.GetActionList().QueueActionNext(Robot::SoundSlot, new PlayAnimationAction(_turningSoundAnimation, 1, false));
      
      _nextSoundTime = currentTime + GetRNG().RandDblInRange(_soundSpacingMin_sec, _soundSpacingMax_sec);
    }
      
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

TrackObjectAction::TrackObjectAction(const ObjectID& objectID, bool trackByType)
: _objectID(objectID)
, _trackByType(trackByType)
{

}

ActionResult TrackObjectAction::Init(Robot& robot)
{
  if(!_objectID.IsSet()) {
    PRINT_NAMED_ERROR("TrackObjectAction.Init.ObjectIdNotSet", "");
    return ActionResult::FAILURE_ABORT;
  }
  
  const ObservableObject* object = robot.GetBlockWorld().GetObjectByID(_objectID);
  if(nullptr == object) {
    PRINT_NAMED_ERROR("TrackObjectAction.Init.InvalidObject",
                      "Object %d does not exist in BlockWorld", _objectID.GetValue());
    return ActionResult::FAILURE_ABORT;
  }
  
  _objectType = object->GetType();
  _lastTrackToPose = object->GetPose();
  
  if(_trackByType) {
    _name = "TrackObject" + std::string(EnumToString(_objectType)) + "Action";
  } else {
    _name = "TrackObject" + std::to_string(_objectID) + "Action";
  }
  
  robot.GetMoveComponent().SetTrackToObject(_objectID);
  
  return ActionResult::SUCCESS;
} // Init()

void TrackObjectAction::Cleanup(Robot& robot)
{
  robot.GetMoveComponent().UnSetTrackToObject();
}
  
bool TrackObjectAction::GetAngles(Robot& robot, Radians& absPanAngle, Radians& absTiltAngle)
{
  ObservableObject* matchingObject = nullptr;
  
  if(_trackByType) {
    BlockWorldFilter filter;
    filter.OnlyConsiderLatestUpdate(true);
    
    matchingObject = robot.GetBlockWorld().FindClosestMatchingObject(_objectType, _lastTrackToPose, 1000.f, DEG_TO_RAD(180), filter);
    
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
      robot.GetMoveComponent().SetTrackToObject(matchingObject->GetID());
    }
  } else {
    matchingObject = robot.GetBlockWorld().GetObjectByID(_objectID);
    if(nullptr == matchingObject) {
      PRINT_NAMED_WARNING("TrackObjectAction.GetAngles.ObjectNoLongerExists",
                          "Object %d no longer exists in BlockWorld",
                          _objectID.GetValue());
      return false;
    }
  }
  
  assert(nullptr != matchingObject);
  
  ASSERT_NAMED(ObservableObject::PoseState::Unknown != matchingObject->GetPoseState(),
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
    if(false == marker->GetPose().GetWithRespectTo(robot.GetPose(), markerPoseWrtRobot)) {
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
  absPanAngle  = std::atan2(yDist, xDist) + robot.GetPose().GetRotation().GetAngleAroundZaxis();
  
  return true;
  
} // GetAngles()
  
  
#pragma mark -
#pragma mark TrackFaceAction
  
TrackFaceAction::TrackFaceAction(FaceID faceID)
: _faceID(faceID)
{
  
}


ActionResult TrackFaceAction::Init(Robot& robot)
{
  _name = "TrackFace" + std::to_string(_faceID) + "Action";
  robot.GetMoveComponent().SetTrackToFace(_faceID);
  _lastFaceUpdate = 0;
  return ActionResult::SUCCESS;
} // Init()
  
  
void TrackFaceAction::Cleanup(Robot& robot)
{
  robot.GetMoveComponent().UnSetTrackToFace();
}

void TrackFaceAction::GetCompletionUnion(Robot& robot, ActionCompletedUnion& completionUnion) const
{
  TrackFaceCompleted completion;
  completion.faceID = static_cast<s32>(_faceID);
  completionUnion.Set_trackFaceCompleted(std::move(completion));
}
  
bool TrackFaceAction::GetAngles(Robot& robot, Radians& absPanAngle, Radians& absTiltAngle)
{
  const Vision::TrackedFace* face = robot.GetFaceWorld().GetFace(_faceID);
  
  if(nullptr == face) {
    // No such face
    return false;
  }
  
  // Only update pose if we've actually observed the face again since last update
  if(face->GetTimeStamp() <= _lastFaceUpdate) {
    return false;
  }
  _lastFaceUpdate = face->GetTimeStamp();
  
  Pose3d headPoseWrtRobot;
  if(false == face->GetHeadPose().GetWithRespectTo(robot.GetPose(), headPoseWrtRobot)) {
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
  
  ASSERT_NAMED(xyDistSq > 0.f, "Distance to tracked face should be > 0.");
  
  absTiltAngle = std::atan(zDist/std::sqrt(xyDistSq));
  absPanAngle  = std::atan2(yDist, xDist) + robot.GetPose().GetRotation().GetAngleAroundZaxis();

  return true;
} // GetAngles()
  
  
#pragma mark -
#pragma mark TrackMotionAction
  
ActionResult TrackMotionAction::Init(Robot& robot)
{
  if(false == robot.HasExternalInterface()) {
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
  
  _signalHandle = robot.GetExternalInterface()->Subscribe(ExternalInterface::MessageEngineToGameTag::RobotObservedMotion, HandleObservedMotion);
  
  return ActionResult::SUCCESS;
} // Init()

  
bool TrackMotionAction::GetAngles(Robot& robot, Radians& absPanAngle, Radians& absTiltAngle)
{
  if(_gotNewMotionObservation && _motionObservation.img_area > 0)
  {
    _gotNewMotionObservation = false;
    
    const Point2f motionCentroid(_motionObservation.img_x, _motionObservation.img_y);
    
    // Note: we start with relative angles here, but make them absolute below.
    robot.GetVisionComponent().GetCamera().ComputePanAndTiltAngles(motionCentroid, absPanAngle, absTiltAngle);
    
    // Find pose of robot at time motion was observed
    RobotPoseStamp* poseStamp = nullptr;
    TimeStamp_t junkTime;
    if(RESULT_OK != robot.GetPoseHistory()->ComputeAndInsertPoseAt(_motionObservation.timestamp, junkTime, &poseStamp)) {
      PRINT_NAMED_ERROR("TrackMotionAction.GetAngles.PoseHistoryError",
                        "Could not get historical pose for motion observed at t=%d (lastRobotMsgTime = %d)",
                        _motionObservation.timestamp,
                        robot.GetLastMsgTimestamp());
      return false;
    }
    
    assert(nullptr != poseStamp);
    
    // Make absolute
    absTiltAngle += poseStamp->GetHeadAngle();
    absPanAngle  += poseStamp->GetPose().GetRotation().GetAngleAroundZaxis();
    
#   if DEBUG_TRACKING_ACTIONS
    PRINT_NAMED_INFO("TrackMotionAction.GetAngles.Motion",
                     "Motion area=%d, centroid=(%.1f,%.1f)",
                     _motionObservation.img_area,
                     motionCentroid.x(), motionCentroid.y());
#   endif
    
    return true;
    
  } // if(_gotNewMotionObservation && _motionObservation.img_area > 0)
  
  return false;
  
} // GetAngles()
  
  
} // namespace Cozmo
} // namespace Anki
