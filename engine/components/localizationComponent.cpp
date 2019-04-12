/**
 * File: localizationComponent.h
 *
 * Author: Michael Willett
 * Created: 3/21/2019
 *
 * Description: Maintains ownership of main localization related tasks
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#include "engine/components/localizationComponent.h"

#include "engine/fullRobotPose.h"
#include "engine/robotStateHistory.h"
#include "engine/cozmoContext.h"
#include "engine/charger.h"

#include "engine/aiComponent/aiComponent.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/pathComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/navMap/mapComponent.h"
#include "engine/externalInterface/externalInterface.h"

#include "engine/blockWorld/blockWorld.h"
#include "engine/faceWorld.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageEngineToRobot.h"

#include "util/logging/DAS.h"
#include "util/logging/logging.h"


#define LOG_CHANNEL "Localization"

namespace Anki {
namespace Vector {
  
LocalizationComponent::LocalizationComponent() 
: IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::Localization)
{
}

LocalizationComponent::~LocalizationComponent()
{
}

void LocalizationComponent::InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps)
{
  _robot = robot;
  _driveCenterPose.SetName("RobotDriveCenter");
}

void LocalizationComponent::UpdateDependent(const RobotCompMap& dependentComps)
{
  if (_needToSendLocalizationUpdate) {
    SendAbsLocalizationUpdate();
    _needToSendLocalizationUpdate = false;
  }
}

Result LocalizationComponent::NotifyOfRobotState(const RobotState& msg) 
{
  Result lastResult = RESULT_OK;

  DEV_ASSERT(msg.pose_frame_id <= GetPoseFrameID(), "Robot.UpdateFullRobotState.FrameFromFuture");
  const bool frameIsCurrent = msg.pose_frame_id == GetPoseFrameID();

  // This is "normal" mode, where we update pose history based on the
  // reported odometry from the physical robot

  // Ignore physical robot's notion of z from the message? (msg.pose_z)
  f32 pose_z = 0.f;


  // Need to put the odometry update in terms of the current robot origin
  if (!_poseOrigins.ContainsOriginID(msg.pose_origin_id))
  {
    LOG_WARNING("Robot.UpdateFullRobotState.BadOriginID",
                "Received RobotState with originID=%u, only %zu pose origins available",
                msg.pose_origin_id, _poseOrigins.GetSize());
    return RESULT_FAIL;
  }

  const Pose3d& origin = _poseOrigins.GetOriginByID(msg.pose_origin_id);

  // Initialize new pose to be within the reported origin
  Pose3d newPose(msg.pose.angle, Z_AXIS_3D(), {msg.pose.x, msg.pose.y, msg.pose.z}, origin);

  // It's possible the pose origin to which this update refers has since been
  // rejiggered and is now the child of another origin. To add to history below,
  // we must first flatten it. We do all this before "fixing" pose_z because pose_z
  // will be w.r.t. robot origin so we want newPose to already be as well.
  newPose = newPose.GetWithRespectToRoot();

  if(msg.pose_frame_id == GetPoseFrameID()) {
    // Frame IDs match. Use the robot's current Z (but w.r.t. world origin)
    pose_z = _robot->GetPose().GetWithRespectToRoot().GetTranslation().z();
  } else {
    // This is an old odometry update from a previous pose frame ID. We
    // need to look up the correct Z value to use for putting this
    // message's (x,y) odometry info into history. Since it comes from
    // pose history, it will already be w.r.t. world origin, since that's
    // how we store everything in pose history.
    HistRobotState histState;
    lastResult = _robot->GetStateHistory()->GetLastStateWithFrameID(msg.pose_frame_id, histState);
    if (lastResult != RESULT_OK) {
      LOG_ERROR("Robot.UpdateFullRobotState.GetLastPoseWithFrameIdError",
                "Failed to get last pose from history with frame ID=%d",
                msg.pose_frame_id);
      return lastResult;
    }
    pose_z = histState.GetPose().GetWithRespectToRoot().GetTranslation().z();
  }

  newPose.SetTranslation({newPose.GetTranslation().x(), newPose.GetTranslation().y(), pose_z});


  // Add to history
  const HistRobotState histState(newPose, msg, _robot->GetProxSensorComponent().GetLatestProxData() );
  lastResult = _robot->GetStateHistory()->AddRawOdomState(msg.timestamp, histState);

  if (lastResult != RESULT_OK) {
    LOG_WARNING("Robot.UpdateFullRobotState.AddPoseError",
                "AddRawOdomStateToHistory failed for timestamp=%d", msg.timestamp);
    return lastResult;
  }

  Pose3d prevDriveCenterPose ;
  _robot->ComputeDriveCenterPose(_robot->GetPose(), prevDriveCenterPose);

  if (UpdateCurrPoseFromHistory() == false) {
    lastResult = RESULT_FAIL;
  }

  if (frameIsCurrent) {
    _numMismatchedFrameIDs = 0;
  } else {
    // COZMO-5850 (Al) This is to catch the issue where our frameID is incremented but fails to send
    // to the robot due to some origin issue. Somehow the robot's pose becomes an origin and doesn't exist
    // in the PoseOriginList. The frameID mismatch then causes all sorts of issues in things (ie VisionSystem
    // won't process the next image). Delocalizing will fix the mismatch by creating a new origin and sending
    // a localization update
    static const u32 kNumTicksWithMismatchedFrameIDs = 100; // 3 seconds (called each RobotState msg)

    ++_numMismatchedFrameIDs;

    if (_numMismatchedFrameIDs > kNumTicksWithMismatchedFrameIDs)
    {
      LOG_ERROR("Robot.UpdateFullRobotState.MismatchedFrameIDs",
                "Robot[%u] and engine[%u] frameIDs are mismatched, delocalizing",
                msg.pose_frame_id,
                GetPoseFrameID());

      Delocalize();

      return RESULT_FAIL;
    }
  }

  return lastResult;
}

void LocalizationComponent::Delocalize()
{
  _isLocalized = false;
  _localizedToID.UnSet();
  _localizedToFixedObject = false;
  _numMismatchedFrameIDs = 0;

  // NOTE: no longer doing this here because Delocalize() can be called by
  //  BlockWorld::ClearAllExistingObjects, resulting in a weird loop...
  //_blockWorld.ClearAllExistingObjects();

  // TODO rsam:
  // origins are no longer destroyed to prevent children from having to rejigger as cubes do. This however
  // has the problem of leaving zombie origins, and having systems never deleting dead poses that can never
  // be transformed withRespectTo a current origin. The origins growing themselves is not a big problem since
  // they are merely a Pose3d instance. However systems that keep Poses around because they have a valid
  // origin could potentially be a problem. This would have to be profiled to identify those systems, so not
  // really worth adding here a warning for "number of zombies is too big" without actually keeping track
  // of how many children they hold, or for how long. Eg: zombies with no children could auto-delete themselves,
  // but is the cost of bookkeeping bigger than what we are currently losing to zombies? That's the question
  // to profile

  // Store a copy of the old origin ID
  const PoseOriginID_t oldOriginID = _poseOrigins.GetCurrentOriginID();

  // Add a new origin
  const PoseOriginID_t worldOriginID = _poseOrigins.AddNewOrigin();
  const Pose3d& worldOrigin = _poseOrigins.GetCurrentOrigin();
  DEV_ASSERT_MSG(worldOriginID == _poseOrigins.GetCurrentOriginID(),
                 "Robot.Delocalize.UnexpectedNewWorldOriginID", "%d vs. %d",
                 worldOriginID, _poseOrigins.GetCurrentOriginID());
  DEV_ASSERT_MSG(worldOriginID == worldOrigin.GetID(),
                 "Robot.Delocalize.MismatchedWorldOriginID", "%d vs. %d",
                 worldOriginID, worldOrigin.GetID());

  // Log delocalization, new origin name, and num origins to DAS
  LOG_INFO("Robot.Delocalize",
           "Delocalizing robot. New origin: %s. NumOrigins=%zu",
           worldOrigin.GetName().c_str(), _poseOrigins.GetSize());

  _robot->GetComponent<FullRobotPose>().GetPose().SetRotation(0, Z_AXIS_3D());
  _robot->GetComponent<FullRobotPose>().GetPose().SetTranslation({0.f, 0.f, 0.f});
  _robot->GetComponent<FullRobotPose>().GetPose().SetParent(worldOrigin);

  _driveCenterPose.SetRotation(0, Z_AXIS_3D());
  _driveCenterPose.SetTranslation({0.f, 0.f, 0.f});
  _driveCenterPose.SetParent(worldOrigin);

  // Create a new pose frame so that we can't get pose history entries with the same pose
  // frame that have different origins (Not 100% sure this is totally necessary but seems
  // like the cleaner / safer thing to do.)
  Result res = SetNewPose(_robot->GetComponent<FullRobotPose>().GetPose());
  if (res != RESULT_OK)
  {
    LOG_WARNING("Robot.Delocalize.SetNewPose", "Failed to set new pose");
  }

  if (_robot->GetSyncRobotAcked())
  {
    // Need to update the robot's pose history with our new origin and pose frame IDs
    LOG_INFO("Robot.Delocalize.SendingNewOriginID",
             "Sending new localization update at t=%u, with pose frame %u and origin ID=%u",
             (TimeStamp_t)_robot->GetLastMsgTimestamp(),
             _frameId, worldOrigin.GetID());
    SendAbsLocalizationUpdate(_robot->GetComponent<FullRobotPose>().GetPose(), _robot->GetLastMsgTimestamp(), _frameId);
  }

  // Update VizText
  auto vizm = _robot->GetContext()->GetVizManager();
  vizm->SetText(TextLabelType::LOCALIZED_TO, NamedColors::YELLOW,
                                         "LocalizedTo: <nothing>");
  vizm->SetText(TextLabelType::WORLD_ORIGIN, NamedColors::YELLOW,
                                         "WorldOrigin[%lu]: %s",
                                         _poseOrigins.GetSize(),
                                         worldOrigin.GetName().c_str());
  vizm->EraseAllVizObjects();

  // Have to do this _after_ clearing the pose confirmer because UpdateObjectOrigin
  // adds the carried objects to the pose confirmer in their newly updated pose,
  // but _before_ deleting zombie objects (since dirty carried objects may get
  // deleted)
  if (_robot->GetCarryingComponent().IsCarryingObject())
  {
    // Carried objects are in the pose chain of the robot, whose origin has now changed.
    // Thus the carried object's actual origin no longer matches the way they are stored
    // in BlockWorld.
    const auto& objectID = _robot->GetCarryingComponent().GetCarryingObjectID();
    const Result result = _robot->GetBlockWorld().UpdateObjectOrigin(objectID, oldOriginID);
    if(RESULT_OK != result)
    {
      LOG_WARNING("Robot.Delocalize.UpdateObjectOriginFailed", "Object %d", objectID.GetValue());
    }
  }

  // If we don't know where we are, we can't know where we are going.
  _robot->GetPathComponent().Abort();
  _robot->GetBlockWorld().OnRobotDelocalized(worldOriginID);
  _robot->GetFaceWorld().OnRobotDelocalized(worldOriginID);
  _robot->GetAIComponent().OnRobotDelocalized();
  _robot->GetMoveComponent().OnRobotDelocalized();

  // send message to game. At the moment I implement this so that Webots can update the render, but potentially
  // any system can listen to this
  _robot->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotDelocalized()));
  
  DASMSG(robot_delocalized,
         "robot.delocalized",
         "The robot has delocalized. This event occurs any time the robot delocalizes.");
  DASMSG_SET(i1, _robot->GetCarryingComponent().IsCarryingObject(), "1 if carrying an object, null if not");
  DASMSG_SEND();

} // Delocalize()

Result LocalizationComponent::SetLocalizedTo(const ObservableObject* object)
{
  if (object == nullptr) {
    _robot->GetContext()->GetVizManager()->SetText(TextLabelType::LOCALIZED_TO, NamedColors::YELLOW,
                                           "LocalizedTo: Odometry");
    _localizedToID.UnSet();
    _isLocalized = true;
    return RESULT_OK;
  }
  
  // Do not allow localizing if we're not on treads
  if (_robot->GetOffTreadsState() != OffTreadsState::OnTreads) {
    LOG_ERROR("Robot.SetLocalizedTo.OffTreads", "Cannot localize while off treads");
    return RESULT_FAIL;
  }

  if (object->GetID().IsUnknown()) {
    LOG_ERROR("Robot.SetLocalizedTo.IdNotSet", "Cannot localize to an object with no ID set");
    return RESULT_FAIL;
  }

  // Find the closest, most recently observed marker on the object
  RobotTimeStamp_t mostRecentObsTime = 0;
  for(const auto& marker : object->GetMarkers()) {
    if(marker.GetLastObservedTime() >= mostRecentObsTime) {
      Pose3d markerPoseWrtCamera;
      if (false == marker.GetPose().GetWithRespectTo(_robot->GetVisionComponent().GetCamera().GetPose(), markerPoseWrtCamera)) {
        LOG_ERROR("Robot.SetLocalizedTo.MarkerOriginProblem", "Could not get pose of marker w.r.t. robot camera");
        return RESULT_FAIL;
      }
      // TODO: I don't know if tracking `_localizedMarkerDistToCameraSq` helps anything here
      // const f32 distToMarkerSq = markerPoseWrtCamera.GetTranslation().LengthSq();
      // if(_localizedMarkerDistToCameraSq < 0.f || distToMarkerSq < _localizedMarkerDistToCameraSq) {
      //   _localizedMarkerDistToCameraSq = distToMarkerSq;
        mostRecentObsTime = marker.GetLastObservedTime();
      // }
    }
  }
  // assert(_localizedMarkerDistToCameraSq >= 0.f);

  _localizedToID = object->GetID();
  _hasMovedSinceLocalization = false;
  _isLocalized = true;

  // notify behavior whiteboard
  _robot->GetAIComponent().OnRobotRelocalized();

  // Update VizText
  _robot->GetContext()->GetVizManager()->SetText(TextLabelType::LOCALIZED_TO, NamedColors::YELLOW,
                                         "LocalizedTo: %s_%d",
                                         ObjectTypeToString(object->GetType()), _localizedToID.GetValue());
  _robot->GetContext()->GetVizManager()->SetText(TextLabelType::WORLD_ORIGIN, NamedColors::YELLOW,
                                         "WorldOrigin[%lu]: %s",
                                         _poseOrigins.GetSize(),
                                         GetWorldOrigin().GetName().c_str());

  DASMSG(robot_localized_to_object, "robot.localized_to_object", "The robot has localized to an object");
  DASMSG_SET(s1, EnumToString(object->GetType()), "object type");
  DASMSG_SET(i1, object->GetPose().GetTranslation().x(), "x coordinate of object pose");
  DASMSG_SET(i2, object->GetPose().GetTranslation().y(), "y coordinate of object pose");
  DASMSG_SET(i3, object->GetPose().GetTranslation().z(), "z coordinate of object pose");
  DASMSG_SET(i4, _robot->GetPose().GetTranslation().z(), "z coordinate of robot pose");
  DASMSG_SEND();
  
  return RESULT_OK;

} // SetLocalizedTo()

Result LocalizationComponent::LocalizeToObject(const ObservableObject* seenObject,
                                               ObservableObject* existingObject)
{
  Result lastResult = RESULT_OK;

  if (existingObject == nullptr) {
    LOG_ERROR("Robot.LocalizeToObject.ExistingObjectPieceNullPointer", "");
    return RESULT_FAIL;
  }
  
  if (!IsChargerType(existingObject->GetType(), false)) {
    LOG_ERROR("Robot.LocalizeToObject.CanOnlyLocalizeToCharger", "");
    return RESULT_FAIL;
  }

  if (existingObject->GetID() != GetLocalizedTo())
  {
    LOG_DEBUG("Robot.LocalizeToObject",
              "Robot attempting to localize to %s object %d",
              EnumToString(existingObject->GetType()),
              existingObject->GetID().GetValue());
  }

  HistStateKey histStateKey;
  HistRobotState* histStatePtr = nullptr;
  Pose3d robotPoseWrtObject;
  float  headAngle;
  float  liftAngle;
  if (nullptr == seenObject)
  {
    if (false == _robot->GetPose().GetWithRespectTo(existingObject->GetPose(), robotPoseWrtObject)) {
      LOG_ERROR("Robot.LocalizeToObject.ExistingObjectOriginMismatch",
                "Could not get robot pose w.r.t. to existing object %d.",
                existingObject->GetID().GetValue());
      return RESULT_FAIL;
    }
    liftAngle = _robot->GetComponent<FullRobotPose>().GetLiftAngle();
    headAngle = _robot->GetComponent<FullRobotPose>().GetHeadAngle();
  } else {
    // Get computed HistRobotState at the time the object was observed.
    if ((lastResult = _robot->GetStateHistory()->GetComputedStateAt(seenObject->GetLastObservedTime(), &histStatePtr, &histStateKey)) != RESULT_OK) {
      LOG_ERROR("Robot.LocalizeToObject.CouldNotFindHistoricalPose", "Time %d", seenObject->GetLastObservedTime());
      return lastResult;
    }

    // The computed historical pose is always stored w.r.t. the robot's world
    // origin and parent chains are lost. Re-connect here so that GetWithRespectTo
    // will work correctly
    Pose3d robotPoseAtObsTime = histStatePtr->GetPose();
    robotPoseAtObsTime.SetParent(GetWorldOrigin());

    // Get the pose of the robot with respect to the observed object
    if (robotPoseAtObsTime.GetWithRespectTo(seenObject->GetPose(), robotPoseWrtObject) == false) {
      LOG_ERROR("Robot.LocalizeToObject.ObjectPoseOriginMisMatch",
                "Could not get HistRobotState w.r.t. seen object pose.");
      return RESULT_FAIL;
    }

    liftAngle = histStatePtr->GetLiftAngle_rad();
    headAngle = histStatePtr->GetHeadAngle_rad();
  }

  // Make the computed robot pose use the existing object as its parent
  robotPoseWrtObject.SetParent(existingObject->GetPose());
  //robotPoseWrtMat.SetName(std::string("Robot_") + std::to_string(robot->GetID()));

  // Add the new vision-based pose to the robot's history. Note that we use
  // the pose w.r.t. the origin for storing poses in history.
  Pose3d robotPoseWrtOrigin = robotPoseWrtObject.GetWithRespectToRoot();

  if (IsLocalized()) {
    // Filter Z so it doesn't change too fast (unless we are switching from
    // delocalized to localized)

    // Make z a convex combination of new and previous value
    static const f32 zUpdateWeight = 0.1f; // weight of new value (previous gets weight of 1 - this)
    Vec3f T = robotPoseWrtOrigin.GetTranslation();
    T.z() = (zUpdateWeight*robotPoseWrtOrigin.GetTranslation().z() +
             (1.f - zUpdateWeight) * _robot->GetPose().GetTranslation().z());
    robotPoseWrtOrigin.SetTranslation(T);
  }

  if (nullptr != seenObject)
  {
    lastResult = AddVisionOnlyStateToHistory(seenObject->GetLastObservedTime(),
                                             robotPoseWrtOrigin,
                                             headAngle, liftAngle);
    if (lastResult != RESULT_OK)
    {
      LOG_ERROR("Robot.LocalizeToObject.FailedAddingVisionOnlyPoseToHistory", "");
      return lastResult;
    }
  }

  // If the robot's world origin is about to change by virtue of being localized
  // to existingObject, rejigger things so anything seen while the robot was
  // rooted to this world origin will get updated to be w.r.t. the new origin.
  const Pose3d& origOrigin = _poseOrigins.GetCurrentOrigin();
  if (!existingObject->GetPose().HasSameRootAs(origOrigin))
  {
    LOG_INFO("Robot.LocalizeToObject.RejiggeringOrigins",
             "Robot's current origin is %s, about to localize to origin %s.",
             origOrigin.GetName().c_str(),
             existingObject->GetPose().FindRoot().GetName().c_str());

    const PoseOriginID_t origOriginID = _poseOrigins.GetCurrentOriginID();

    // Update the origin to which _worldOrigin currently points to contain
    // the transformation from its current pose to what is about to be the
    // robot's new origin.
    Transform3d transform(_robot->GetPose().GetTransform().GetInverse());
    transform.PreComposeWith(robotPoseWrtOrigin.GetTransform());

    Result result = _poseOrigins.Rejigger(robotPoseWrtObject.FindRoot(), transform);
    if (ANKI_VERIFY(RESULT_OK == result, "Robot.LocalizeToObject.RejiggerFailed", ""))
    {
      const PoseOriginID_t newOriginID = _poseOrigins.GetCurrentOriginID();

      // Now we need to go through all objects whose poses have been adjusted
      // by this origin switch and notify the outside world of the change.
      // Note that map component must be updated before blockworld in case blockworld
      // tries to insert a new object into the map.
      _robot->GetMapComponent().UpdateMapOrigins(origOriginID, newOriginID);
      _robot->GetBlockWorld().UpdateObjectOrigins(origOriginID, newOriginID);
      _robot->GetFaceWorld().UpdateFaceOrigins(origOriginID, newOriginID);

      // after updating all block world objects, flatten out origins to remove grandparents
      _poseOrigins.Flatten(newOriginID);
    }
  }


  if (nullptr != histStatePtr)
  {
    // Update the computed historical pose as well so that subsequent block
    // pose updates use obsMarkers whose camera's parent pose is correct.
    histStatePtr->SetPose(GetPoseFrameID(), robotPoseWrtOrigin, headAngle, liftAngle);
  }


  // Compute the new "current" pose from history which uses the
  // past vision-based "ground truth" pose we just computed.
  DEV_ASSERT_MSG(existingObject->GetPose().HasSameRootAs(GetWorldOrigin()),
                 "Robot.LocalizeToObject.ExistingObjectHasWrongOrigin",
                 "ObjectOrigin:%s WorldOrigin:%s",
                 existingObject->GetPose().FindRoot().GetName().c_str(),
                 GetWorldOrigin().GetName().c_str());

  if (UpdateCurrPoseFromHistory() == false) {
    LOG_ERROR("Robot.LocalizeToObject.FailedUpdateCurrPoseFromHistory", "");
    return RESULT_FAIL;
  }

  if (histStatePtr->WasPickedUp() || (_robot->GetOffTreadsState() != OffTreadsState::OnTreads)) {
    LOG_INFO("Robot.LocalizeToObject.OffTreads", "Not localizing to object since we are not on treads");
    return RESULT_OK;
  }
  
  // Mark the robot as now being localized to this object
  // NOTE: this should be _after_ calling AddVisionOnlyStateToHistory, since
  //    that function checks whether the robot is already localized
  lastResult = SetLocalizedTo(existingObject);
  if (RESULT_OK != lastResult) {
    LOG_ERROR("Robot.LocalizeToObject.SetLocalizedToFail", "");
    return lastResult;
  }

  // Overly-verbose. Use for debugging localization issues
  /*
    LOG_INFO("Robot.LocalizeToObject",
    "Using %s object %d to localize robot %d at (%.3f,%.3f,%.3f), %.1fdeg@(%.2f,%.2f,%.2f), frameID=%d\n",
    ObjectTypeToString(existingObject->GetType()),
    existingObject->GetID().GetValue(), GetID(),
    GetPose().GetTranslation().x(),
    GetPose().GetTranslation().y(),
    GetPose().GetTranslation().z(),
    GetPose().GetRotationAngle<'Z'>().getDegrees(),
    GetPose().GetRotationAxis().x(),
    GetPose().GetRotationAxis().y(),
    GetPose().GetRotationAxis().z(),
    GetPoseFrameID());
  */

  // Don't actually send the update here, because it's possible that we are going to
  // call LocalizeToObject more than once in this tick, which could cause the pose
  // frame ID to update multiple times, replacing what's stored for this timestamp
  // in pose history. We don't want to send a pose frame to the robot that's about
  // to be replaced in history and never used again (causing errors in looking for it
  // when we later receive a RobotState message with that invalidated frameID), so just
  // set this flag to do the localization update once out in Update().
  _needToSendLocalizationUpdate = true;

  return RESULT_OK;
} // LocalizeToObject()



Result LocalizationComponent::SendAbsLocalizationUpdate(const Pose3d&             pose,
                                        const RobotTimeStamp_t&   t,
                                        const PoseFrameID_t&      frameId) const
{
  // Send flattened poses to the robot, because when we get them back in odometry
  // updates with origin IDs, we can only hook them back up directly to the
  // corresponding pose origin (we can't know the chain that led there anymore)

  const Pose3d& poseWrtOrigin = pose.GetWithRespectToRoot();
  const Pose3d& origin = poseWrtOrigin.GetParent(); // poseWrtOrigin's parent is, by definition, the root / an origin
  DEV_ASSERT(origin.IsRoot(), "Robot.SendAbsLocalizationUpdate.OriginNotRoot");
  DEV_ASSERT(pose.HasSameRootAs(origin), "Robot.SendAbsLocalizationUpdate.ParentOriginMismatch");

  const PoseOriginID_t originID = origin.GetID();
  if (!_poseOrigins.ContainsOriginID(originID))
  {
    LOG_ERROR("Robot.SendAbsLocalizationUpdate.InvalidPoseOriginID",
              "Origin %d(%s)", originID, origin.GetName().c_str());
    return RESULT_FAIL;
  }

  return _robot->SendMessage(RobotInterface::EngineToRobot(
                       RobotInterface::AbsoluteLocalizationUpdate(
                         (TimeStamp_t)t,
                         frameId,
                         originID,
                         poseWrtOrigin.GetTranslation().x(),
                         poseWrtOrigin.GetTranslation().y(),
                         poseWrtOrigin.GetRotation().GetAngleAroundZaxis().ToFloat()
                         )));
}

Result LocalizationComponent::SendAbsLocalizationUpdate() const
{
  // Look in history for the last vis pose and send it.
  RobotTimeStamp_t t;
  HistRobotState histState;
  if (_robot->GetStateHistory()->GetLatestVisionOnlyState(t, histState) == RESULT_FAIL) {
    LOG_WARNING("Robot.SendAbsLocUpdate.NoVizPoseFound", "");
    return RESULT_FAIL;
  }

  return SendAbsLocalizationUpdate(histState.GetPose().GetWithRespectToRoot(), t, histState.GetFrameId());
}

Result LocalizationComponent::AddVisionOnlyStateToHistory(const RobotTimeStamp_t t,
                                          const Pose3d& pose,
                                          const f32 head_angle,
                                          const f32 lift_angle)
{
  // We have a new ("ground truth") key frame. Increment the pose frame!
  ++_frameId;

  // Set needToSendLocalizationUpdate to true so we send an update on the next tick
  _needToSendLocalizationUpdate = true;

  HistRobotState histState;
  histState.SetPose(_frameId, pose, head_angle, lift_angle);
  return _robot->GetStateHistory()->AddVisionOnlyState(t, histState);
}

Result LocalizationComponent::SetNewPose(const Pose3d& newPose)
{
  SetPose(newPose.GetWithRespectToRoot());

  // Note: using last message timestamp instead of newest timestamp in history
  //  because it's possible we did not put the last-received state message into
  //  history (if it had old frame ID), but we still want the latest time we
  //  can get.
  const RobotTimeStamp_t timeStamp = _robot->GetLastMsgTimestamp();

  const auto& robotPose = _robot->GetComponent<FullRobotPose>();
  return AddVisionOnlyStateToHistory(timeStamp, robotPose.GetPose(), robotPose.GetHeadAngle(), robotPose.GetLiftAngle());
}

void LocalizationComponent::SetPose(const Pose3d &newPose)
{
  // The new pose should have our current world origin as its origin
  if (!ANKI_VERIFY(newPose.HasSameRootAs(GetWorldOrigin()),
                  "Robot.SetPose.NewPoseOriginAndWorldOriginMismatch",
                  ""))
  {
    return;
  }

  // Update our current pose and keep the name consistent
  const std::string name = _robot->GetComponent<FullRobotPose>().GetPose().GetName();
  _robot->GetComponent<FullRobotPose>().SetPose(newPose);
  _robot->GetComponent<FullRobotPose>().GetPose().SetName(name);

  _robot->ComputeDriveCenterPose(_robot->GetComponent<FullRobotPose>().GetPose(), _driveCenterPose);

} // SetPose()


Result LocalizationComponent::SetPoseOnCharger()
{
  // ANKI_CPU_PROFILE("Robot::SetPoseOnCharger");

  Charger* charger = dynamic_cast<Charger*>(_robot->GetBlockWorld().GetLocatedObjectByID(_chargerID));
  if (charger == nullptr) {
    LOG_WARNING("Robot.SetPoseOnCharger.NoChargerWithID",
                "Robot has docked to charger, but Charger object with ID=%d not found in the world.",
                _chargerID.GetValue());
    return RESULT_FAIL;
  }

  // Just do an absolute pose update, setting the robot's position to
  // where we "know" he should be when he finishes ascending the charger.
  Result lastResult = SetNewPose(charger->GetRobotDockedPose().GetWithRespectToRoot());
  if (lastResult != RESULT_OK) {
    LOG_WARNING("Robot.SetPoseOnCharger.SetNewPose", "Robot failed to set new pose");
    return lastResult;
  }

  const RobotTimeStamp_t timeStamp = _robot->GetStateHistory()->GetNewestTimeStamp();

  LOG_INFO("Robot.SetPoseOnCharger.SetPose",
           "Robotnow on charger %d, at (%.1f,%.1f,%.1f) @ %.1fdeg, timeStamp = %d",
           charger->GetID().GetValue(),
           _robot->GetComponent<FullRobotPose>().GetPose().GetTranslation().x(), _robot->GetComponent<FullRobotPose>().GetPose().GetTranslation().y(), _robot->GetComponent<FullRobotPose>().GetPose().GetTranslation().z(),
           _robot->GetComponent<FullRobotPose>().GetPose().GetRotationAngle<'Z'>().getDegrees(),
            (TimeStamp_t)timeStamp);

  return RESULT_OK;

} // SetPoseOnCharger()

Result LocalizationComponent::SetPosePostRollOffCharger()
{
  auto* charger = dynamic_cast<Charger*>(_robot->GetBlockWorld().GetLocatedObjectByID(_chargerID));
  if (charger == nullptr) {
    LOG_WARNING("Robot.SetPosePostRollOffCharger.NoChargerWithID",
                "Charger object with ID %d not found in the world.",
                _chargerID.GetValue());
    return RESULT_FAIL;
  }

  // Just do an absolute pose update, setting the robot's position to
  // where we "know" he should be when he finishes rolling off the charger.
  Result lastResult = SetNewPose(charger->GetRobotPostRollOffPose().GetWithRespectToRoot());
  if (lastResult != RESULT_OK) {
    LOG_WARNING("Robot.SetPosePostRollOffCharger.SetNewPose", "Failed to set new pose");
    return lastResult;
  }

  LOG_INFO("Robot.SetPosePostRollOffCharger.NewRobotPose",
                   "Updated robot pose to be in front of the charger, as if it had just rolled off.");
  return RESULT_OK;
}

bool LocalizationComponent::UpdateCurrPoseFromHistory()
{
  bool poseUpdated = false;

  RobotTimeStamp_t t;
  HistRobotState histState;
  // TODO: Just get StateHistory().GetLastStateWithFrameID() ? 
  if (_robot->GetStateHistory()->ComputeStateAt(_robot->GetStateHistory()->GetNewestTimeStamp(), t, histState) == RESULT_OK)
  {
    const Pose3d& worldOrigin = GetWorldOrigin();
    Pose3d newPose;
    if ((histState.GetPose().GetWithRespectTo(worldOrigin, newPose))==false)
    {
      // This is not necessarily an error anymore: it's possible we've received an
      // odometry update from the robot w.r.t. an old origin (before being delocalized),
      // in which case we can't use it to update the current pose of the robot
      // in its new frame.
      LOG_INFO("Robot.UpdateCurrPoseFromHistory.GetWrtParentFailed",
               "Could not update robot's current pose using historical pose w.r.t. %s because we are now in frame %s.",
               histState.GetPose().FindRoot().GetName().c_str(),
               worldOrigin.GetName().c_str());
    }
    else
    {
      SetPose(newPose);
      poseUpdated = true;
    }

  }

  return poseUpdated;
}

} // Cozmo namespace
} // Anki namespace
