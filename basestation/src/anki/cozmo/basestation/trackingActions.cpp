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
  Vec3f trackingVector;
  if(GetTrackingVector(robot, trackingVector))
  {
    const f32 distSq = trackingVector.LengthSq();
    if(trackingVector.LengthSq() > 0)
    {
      const Radians tiltAngle = std::atan(trackingVector.z()/std::sqrt(distSq));
      const Radians panAngle  = std::atan2(trackingVector.y(), trackingVector.x());
      
      // Pan and/or Tilt (normal behavior)
      if(Mode::HeadAndBody == _mode || Mode::HeadOnly == _mode)
      {
        // TODO: Set speed/accel based on angle difference
        if(RESULT_OK != robot.GetMoveComponent().MoveHeadToAngle(tiltAngle.ToFloat(), 15.f, 20.f))
        {
          return ActionResult::FAILURE_ABORT;
        }
      }
      
      if(Mode::HeadAndBody == _mode || Mode::BodyOnly == _mode)
      {
        RobotInterface::SetBodyAngle setBodyAngle;
        setBodyAngle.angle_rad             = panAngle.ToFloat();
        setBodyAngle.max_speed_rad_per_sec = DEFAULT_PATH_SPEED_MMPS; // TODO: Set based on angle difference
        setBodyAngle.accel_rad_per_sec2    = DEFAULT_PATH_ACCEL_MMPS2; // TODO: Set based on angle difference
        if(RESULT_OK != robot.SendRobotMessage<RobotInterface::SetBodyAngle>(std::move(setBodyAngle))) {
          return ActionResult::FAILURE_ABORT;
        }
      }
      
    } else {
      PRINT_NAMED_WARNING("ITrackAction.CheckIfDone.ZeroLengthTracking",
                          "Ignoring tracking vector with zero length");
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
  
  return ActionResult::SUCCESS;
} // Init()

  
bool TrackObjectAction::GetTrackingVector(Robot& robot, Vec3f& newTrackingVector)
{
  ObservableObject* matchingObject = nullptr;
  
  if(_trackByType) {
    matchingObject = robot.GetBlockWorld().FindClosestMatchingObject(_objectType, _lastTrackToPose, 1000.f, DEG_TO_RAD(360));
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
  
  //const Vec3f& robotTrans = _robot->GetPose().GetTranslation();
  
  // Compare to the pose of the robot when the marker was observed
  RobotPoseStamp *p;
  assert(nullptr != robot.GetPoseHistory());
  if(RESULT_OK != robot.GetPoseHistory()->GetComputedPoseAt(matchingObject->GetLastObservedTime(), &p)) {
    PRINT_NAMED_ERROR("TrackObjectAction.GetAngles.PoseHistoryError",
                      "Could not get historical pose for object observation time t=%d",
                      matchingObject->GetLastObservedTime());
    return false;
  }
  
  const Vec3f& robotTrans = p->GetPose().GetTranslation();
  
  const Vision::KnownMarker* closestMarker = nullptr;
  f32 minDistSq = std::numeric_limits<f32>::max();
  f32 xDist = 0.f, yDist = 0.f, zDist = 0.f;
  
  for(auto marker : observedMarkers) {
    Pose3d markerPose;
    if(false == marker->GetPose().GetWithRespectTo(*robot.GetWorldOrigin(), markerPose)) {
      PRINT_NAMED_ERROR("TrackObjectAction.GetAngles.PoseOriginError",
                        "Could not get pose of observed marker w.r.t. world");
      return false;
    }
    
    const f32 xDist_crnt = markerPose.GetTranslation().x() - robotTrans.x();
    const f32 yDist_crnt = markerPose.GetTranslation().y() - robotTrans.y();
    
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
      zDist = markerPose.GetTranslation().z() - (robotTrans.z() + NECK_JOINT_POSITION[2]);
    }
    
  } // For all markers
  
  if(closestMarker == nullptr) {
    PRINT_NAMED_ERROR("TrackObjectAction.GetAngles.NoClosestMarker", "");
    return false;
  }
  
  newTrackingVector = {xDist, yDist, zDist};
  
  return true;
  
} // GetTrackingVector()
  
  
#pragma mark -
#pragma mark TrackFaceAction
  
  TrackFaceAction::TrackFaceAction(FaceID faceID)
  : _faceID(faceID)
  {
    
  }
  
  ActionResult TrackFaceAction::Init(Robot& robot)
  {
    _name = "TrackFace" + std::to_string(_faceID) + "Action";
    
    return ActionResult::SUCCESS;
  } // Init()
  
  
  bool TrackFaceAction::GetTrackingVector(Robot& robot, Vec3f& newTrackingVector)
  {
    const Vision::TrackedFace* face = robot.GetFaceWorld().GetFace(_faceID);
    
    if(nullptr == face) {
      // No such face
      return false;
    }
    
    // Compare to the pose of the robot when the marker was observed
    RobotPoseStamp *p;
    TimeStamp_t junkTime;
    if(RESULT_OK != robot.GetPoseHistory()->ComputeAndInsertPoseAt(face->GetTimeStamp(), junkTime, &p)) {
      PRINT_NAMED_ERROR("TrackFaceAction.GetTrackingVector.PoseHistoryError",
                        "Could not get historical pose for face observed at t=%d (lastRobotMsgTime = %d)",
                        face->GetTimeStamp(),
                        robot.GetLastMsgTimestamp());
      return false;
    }
    
    const Vec3f& robotTrans = p->GetPose().GetTranslation();
    
    Pose3d headPose;
    if(false == face->GetHeadPose().GetWithRespectTo(*robot.GetWorldOrigin(), headPose)) {
      PRINT_NAMED_ERROR("TrackFaceAction.GetTrackingVector.PoseOriginError",
                        "Could not get pose of face w.r.t. world for head tracking.\n");
      return false;
    }
    
    const f32 xDist = headPose.GetTranslation().x() - robotTrans.x();
    const f32 yDist = headPose.GetTranslation().y() - robotTrans.y();
    
    // NOTE: This isn't perfectly accurate since it doesn't take into account the
    // the head angle and is simply using the neck joint (which should also
    // probably be queried from the robot instead of using the constant here)
    const f32 zDist = headPose.GetTranslation().z() - (robotTrans.z() + NECK_JOINT_POSITION[2]);
    
    newTrackingVector = {xDist, yDist, zDist};
    
    return true;
  } // GetTrackingVector()
  
  
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
  
  
  bool TrackMotionAction::GetTrackingVector(Robot& robot, Vec3f& newTrackingVector)
  {
    if(_gotNewMotionObservation && _motionObservation.img_area > 0)
    {
      _gotNewMotionObservation = false;
      
      const Point2f motionCentroid(_motionObservation.img_x, _motionObservation.img_y);
      
      newTrackingVector = {-motionCentroid.x(), -motionCentroid.y(), robot.GetVisionComponent().GetCameraCalibration().GetFocalLength_x()};
      
      
      PRINT_NAMED_INFO("TrackMotionAction.GetTrackingVector.Motion",
                       "Motion area=%d, centroid=(%.1f,%.1f)",
                       _motionObservation.img_area,
                       motionCentroid.x(), motionCentroid.y());
  
      return true;
      
    } // if(_gotNewMotionObservation && _motionObservation.img_area > 0)
    
    return false;
    
  } // GetTrackingVector()
  
  
} // namespace Cozmo
} // namespace Anki
