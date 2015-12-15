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
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"

#include "clad/externalInterface/messageEngineToGameTag.h"
#include "clad/externalInterface/messageEngineToGame.h"

#include "anki/common/basestation/utils/timer.h"

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

ActionResult ITrackAction::CheckIfDone(Robot& robot)
{
  Radians absPanAngle = 0, absTiltAngle = 0;
  
  // See if there are new relative pan/tilt angles from the derived class
  if(GetAngles(robot, absPanAngle, absTiltAngle))
  {
    // Record latest update to avoid timing out
    if(_timeout_sec > 0.) {
      _lastUpdateTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    }
    
    PRINT_NAMED_INFO("ITrackAction.CheckIfDone.NewAngles",
                     "Commanding abs angles: pan=%.1fdeg, tilt=%.1fdeg",
                     absPanAngle.getDegrees(), absTiltAngle.getDegrees());
    
    // Pan and/or Tilt (normal behavior)
    if(Mode::HeadAndBody == _mode || Mode::HeadOnly == _mode)
    {
      // TODO: Set speed/accel based on angle difference
      if(RESULT_OK != robot.GetMoveComponent().MoveHeadToAngle(absTiltAngle.ToFloat(), 15.f, 20.f))
      {
        return ActionResult::FAILURE_ABORT;
      }
    }
    
    if(Mode::HeadAndBody == _mode || Mode::BodyOnly == _mode)
    {
      // Get rotation angle around drive center
      Pose3d rotatedPose;
      Pose3d dcPose = robot.GetDriveCenterPose();
      dcPose.SetRotation(absPanAngle, Z_AXIS_3D());
      robot.ComputeOriginPose(dcPose, rotatedPose);
      
      RobotInterface::SetBodyAngle setBodyAngle;
      setBodyAngle.angle_rad             = rotatedPose.GetRotation().GetAngleAroundZaxis().ToFloat();
      setBodyAngle.max_speed_rad_per_sec = DEFAULT_PATH_SPEED_MMPS; // TODO: Set based on angle difference
      setBodyAngle.accel_rad_per_sec2    = DEFAULT_PATH_ACCEL_MMPS2; // TODO: Set based on angle difference
      if(RESULT_OK != robot.SendRobotMessage<RobotInterface::SetBodyAngle>(std::move(setBodyAngle))) {
        return ActionResult::FAILURE_ABORT;
      }
    }
      
  } else if(_timeout_sec > 0.) {
    const double currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if(currentTime - _lastUpdateTime > _timeout_sec) {
      PRINT_NAMED_INFO("ITrackAction.CheckIfDone.Timeout",
                       "No tracking angle update received in %f seconds, returning done.",
                       _timeout_sec);
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
      PRINT_NAMED_WARNING("TrackObjectAction.GetAngles.NoMatchingFound",
                          "Could not find matching %s object.",
                          EnumToString(_objectType));
      return false;
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
  
  if(ObservableObject::PoseState::Unknown == matchingObject->GetPoseState()) {
    PRINT_NAMED_WARNING("TrackObjectAction.GetAngles.PoseStateUnknown",
                        "Object %d's pose state is unknown. Cannot update angles.",
                        _objectID.GetValue());
    return false;
  }
  
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
  
  if(minDistSq <= 0.f) {
    return false;
  }
  
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
  
  return ActionResult::SUCCESS;
} // Init()
  
  
void TrackFaceAction::Cleanup(Robot& robot)
{
  robot.GetMoveComponent().UnSetTrackToFace();
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
    PRINT_NAMED_ERROR("TrackFaceAction.GetTrackingVector.PoseOriginError",
                      "Could not get pose of face w.r.t. robot.");
    return false;
  }
  
  const f32 xDist = headPoseWrtRobot.GetTranslation().x();
  const f32 yDist = headPoseWrtRobot.GetTranslation().y();
  
  // NOTE: This isn't perfectly accurate since it doesn't take into account the
  // the head angle and is simply using the neck joint (which should also
  // probably be queried from the robot instead of using the constant here)
  const f32 zDist = headPoseWrtRobot.GetTranslation().z() - NECK_JOINT_POSITION[2];

  PRINT_NAMED_INFO("TrackFaceAction.GetAngles.HeadPose",
                   "Translation w.r.t. robot = (%.1f, %.1f, %.1f) [t=%d]",
                   xDist, yDist, zDist, face->GetTimeStamp());
  
  const f32 xyDistSq = xDist*xDist + yDist*yDist;
  
  if(xyDistSq <= 0.f) {
    return false;
  }
  
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
    
    const Vision::CameraCalibration& calibration = robot.GetVisionComponent().GetCameraCalibration();
    absTiltAngle = std::atan(-motionCentroid.y() / calibration.GetFocalLength_y());
    absPanAngle  = std::atan(-motionCentroid.x() / calibration.GetFocalLength_x());
    
    // Find pose of robot at time motion was observed
    RobotPoseStamp* poseStamp = nullptr;
    TimeStamp_t junkTime;
    if(RESULT_OK != robot.GetPoseHistory()->ComputeAndInsertPoseAt(_motionObservation.timestamp, junkTime, &poseStamp)) {
      PRINT_NAMED_ERROR("TrackMotionAction.GetTrackingVector.PoseHistoryError",
                        "Could not get historical pose for motion observed at t=%d (lastRobotMsgTime = %d)",
                        _motionObservation.timestamp,
                        robot.GetLastMsgTimestamp());
      return false;
    }
    
    assert(nullptr != poseStamp);
    
    // Make absolute
    absTiltAngle += poseStamp->GetHeadAngle();
    absPanAngle  += poseStamp->GetPose().GetRotation().GetAngleAroundZaxis();
    
    PRINT_NAMED_INFO("TrackMotionAction.GetTrackingVector.Motion",
                     "Motion area=%d, centroid=(%.1f,%.1f)",
                     _motionObservation.img_area,
                     motionCentroid.x(), motionCentroid.y());

    return true;
    
  } // if(_gotNewMotionObservation && _motionObservation.img_area > 0)
  
  return false;
  
} // GetAngles()
  
  
} // namespace Cozmo
} // namespace Anki
