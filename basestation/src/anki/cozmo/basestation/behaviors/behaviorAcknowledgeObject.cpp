/**
 * File: behaviorAcknowledgeObject.cpp
 *
 * Author:  Andrew Stein
 * Created: 2016-06-14
 *
 * Description:  Simple quick reaction to a "new" block, just to show Cozmo has noticed it.
 *               Cozmo just turns towards the object and then plays a reaction animation if
 *               it still sees the object after the turn.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorAcknowledgeObject.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "anki/cozmo/basestation/actions/visuallyVerifyActions.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageFromActiveObject.h"
#include "util/console/consoleInterface.h"

#include <limits>

namespace Anki {
namespace Cozmo {

namespace {
CONSOLE_VAR(bool, kEnableObjectAcknowledgement, "BehaviorAcknowledgeObject", true);
CONSOLE_VAR(bool, kVizPossibleStackCube, "BehaviorAcknowledgeObject", false);
}

BehaviorAcknowledgeObject::BehaviorAcknowledgeObject(Robot& robot, const Json::Value& config)
: IBehaviorPoseBasedAcknowledgement(robot, config)
, _ghostStackedObject(new ActiveCube(ObservableObject::InvalidActiveID,
                                     ObservableObject::InvalidFactoryID,
                                     ActiveObjectType::OBJECT_CUBE1))
{
  SetDefaultName("AcknowledgeObject");

  // give the ghost object and ID so we can visualize it
  _ghostStackedObject->SetVizManager(robot.GetContext()->GetVizManager());
  
  SubscribeToTags({
    EngineToGameTag::RobotMarkedObjectPoseUnknown,
  });

  SubscribeToTriggerTags({
    EngineToGameTag::RobotObservedObject,
  });
}

  
Result BehaviorAcknowledgeObject::InitInternalReactionary(Robot& robot)
{
  // don't actually init until the first Update call. This gives other messages that came in this tick a
  // chance to be processed, in case we see multiple objects in the same tick.
  _shouldStart = true;

  // update the id of the temporary cube so it has a valid id (that isn't 0)
  if( !_ghostStackedObject->GetID().IsSet() ) {
    _ghostStackedObject->SetID();
    PRINT_NAMED_DEBUG("BehaviorAcknowledgeObject.GhostObjectCreated",
                      "Created ghost object with id %d for debug viz",
                      _ghostStackedObject->GetID().GetValue());
  }
  
  return Result::RESULT_OK;
}

IBehavior::Status BehaviorAcknowledgeObject::UpdateInternal(Robot& robot)
{
  if( _shouldStart ) {
    _shouldStart = false;
    // now figure out which object to react to
    BeginIteration(robot);
  }

  return super::UpdateInternal(robot);
}

void BehaviorAcknowledgeObject::BeginIteration(Robot& robot)
{
  _currTarget.UnSet();  
  s32 bestTarget = 0;
  if( GetBestTarget(robot, bestTarget) ) {
    _currTarget = bestTarget;
    ASSERT_NAMED(_currTarget.IsSet(), "BehaviorAcknowledgeObject.GotUnsetTarget");
  }
  else {
    return;
  }

  TurnTowardsObjectAction* turnAction = new TurnTowardsObjectAction(robot, _currTarget,
                                                                    _params.maxTurnAngle_rad);
  turnAction->SetTiltTolerance(_params.tiltTolerance_rad);
  turnAction->SetPanTolerance(_params.panTolerance_rad);

  CompoundActionSequential* action = new CompoundActionSequential(robot);
  action->AddAction(turnAction);

  if( _params.numImagesToWaitFor > 0 ) {
    // If visual verification fails (e.g. because object has moved since we saw it),
    // then the compound action fails, and we don't play animation, since it's silly
    // to react to something that's no longer there.
    VisuallyVerifyObjectAction* verifyAction = new VisuallyVerifyObjectAction(robot, _currTarget);
    verifyAction->SetNumImagesToWaitFor(_params.numImagesToWaitFor);
    action->AddAction(verifyAction);
  }

  action->AddAction(new TriggerAnimationAction(robot, _params.reactionAnimTrigger));


  StartActing(action, &BehaviorAcknowledgeObject::LookUpForStackedCube);
}

void BehaviorAcknowledgeObject::LookUpForStackedCube(Robot& robot)
{  
  // look up to check if there is a cube stacked on top
  // TODO:(bn) this should be a DriveToVerifyObject or something like that, once that action exists
  ObservableObject* obj = robot.GetBlockWorld().GetObjectByID(_currTarget);
  if( nullptr == obj ) {
    PRINT_NAMED_WARNING("BehaviorAcknowledgeObject.StackedCube.NullTargetObject",
                        "Target object %d returned null from blockworld",
                        _currTarget.GetValue());
    FinishIteration(robot);
    return;
  }
  else {

    // set up ghost object to represent the one that could be on top of obj
    // pose is relative to obj, but higher
    Pose3d ghostPose = obj->GetPose().GetWithRespectToOrigin();
    ghostPose.SetTranslation({
        ghostPose.GetTranslation().x(),
        ghostPose.GetTranslation().y(),
        ghostPose.GetTranslation().z() + obj->GetSize().z()});
    _ghostStackedObject->SetPose(ghostPose);

    if( kVizPossibleStackCube ) {
      _ghostStackedObject->Visualize(NamedColors::WHITE);
    }
    
    // check if this fake object is theoretically visible from our current position
    static constexpr float kMaxNormalAngle = DEG_TO_RAD(45); // how steep of an angle we can see // ANDREW: is this true?
    static constexpr float kMinImageSizePix = 0.0f; // just check if we are looking at it, size doesn't matter

    // it's ok if markers have nothing behind them, or even if they are occluded. What we want to know is if
    // we'd gain any information by trying to look at the marker pose
    static const std::set<Vision::KnownMarker::NotVisibleReason> okReasons{{
        Vision::KnownMarker::NotVisibleReason::IS_VISIBLE,
        Vision::KnownMarker::NotVisibleReason::OCCLUDED,
        Vision::KnownMarker::NotVisibleReason::NOTHING_BEHIND }};
    
    Vision::KnownMarker::NotVisibleReason reason = _ghostStackedObject->IsVisibleFromWithReason(
      robot.GetVisionComponent().GetCamera(),
      kMaxNormalAngle,
      kMinImageSizePix,
      false);

    if( okReasons.find(reason) != okReasons.end() ) {
      PRINT_NAMED_DEBUG("BehaviorAcknowledgeObject.StackedCube.AlreadyVisible",
                        "Any stacked cube that exists should already be visible (reason %s), nothing to do",
                        NotVisibleReasonToString(reason));
      FinishIteration(robot);
    }
    else {
      // not visible, so we need to look up
      PRINT_NAMED_DEBUG("BehaviorAcknowledgeObject.StackedCube.NotVisible",
                        "looking up in case there is another cube on top of this one (reason %s)",
                        NotVisibleReasonToString(reason));

      // use 0 for max turn angle so we only look with the head

      TurnTowardsObjectAction* turnAction = new TurnTowardsObjectAction(robot, ObjectID{}, 0.f);
      turnAction->UseCustomObject(_ghostStackedObject.get());

      // turn and then wait for images to give us a chance to see the new marker
      CompoundActionSequential* action = new CompoundActionSequential(robot, {
          turnAction,
          new WaitForImagesAction(robot, _params.numImagesToWaitFor)});
      
      StartActing(action, &BehaviorAcknowledgeObject::FinishIteration);
    }
  }
}

void BehaviorAcknowledgeObject::FinishIteration(Robot& robot)
{
  // inform parent class that we completed a reaction
  RobotReactedToId(robot, _currTarget.GetValue());

  // move on to the next target, if there is one
  BeginIteration(robot);
}
 
void BehaviorAcknowledgeObject::StopInternalAcknowledgement(Robot& robot)
{
  // if we get interrupted for any reason, kill the queue. We don't want to back up a bunch of stuff in here,
  // this is meant to handle seeing new objects while we are running
  _currTarget.UnSet();
  _shouldStart = false;
}

bool BehaviorAcknowledgeObject::IsRunnableInternalReactionary(const Robot& robot) const
{
  return kEnableObjectAcknowledgement &&
    !robot.IsCarryingObject() &&
    !robot.IsPickingOrPlacing() &&
    !robot.IsOnCharger() &&
    !robot.IsOnChargerPlatform();
}

void BehaviorAcknowledgeObject::AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotObservedObject: {
      // only update target blocks if we are running
      HandleObjectObserved(robot, event.GetData().Get_RobotObservedObject());
      break;
    }

    case EngineToGameTag::RobotMarkedObjectPoseUnknown:
      HandleObjectMarkedUnknown(robot, event.GetData().Get_RobotMarkedObjectPoseUnknown().objectID);
      break;

    default:
      PRINT_NAMED_ERROR("BehaviorAcknowledgeObject.HandleWhileNotRunning.InvalidTag",
                        "Received event with unhandled tag %hhu.",
                        event.GetData().GetTag());
      break;
  }
} // AlwaysHandleInternal()

void BehaviorAcknowledgeObject::HandleObjectObserved(const Robot& robot,
                                                     const ExternalInterface::RobotObservedObject& msg)
{
  // NOTE: this may get called twice (once from ShouldConsiderObservedObjectHelper and once from
  // AlwaysHandleInternal)

  if( !IsRunnableInternalReactionary(robot) ) {
    // this let's us react as soon as e.g. we come off the charger
    return;
  }

  // Only objects whose marker we actually see are valid
  if( ! msg.markersVisible ) {
    return;
  }
    
  // Object must be in one of the families this behavior cares about
  const bool hasValidFamily = _objectFamilies.count(msg.objectFamily) > 0;
  if(!hasValidFamily) {
    return;
  }

  // check if we want to react based on pose and cooldown, and also update position data even if we don't
  // react
  Pose3d obsPose( msg.pose, robot.GetPoseOriginList() );

  // ignore cubes we are carrying or docking to (don't react to them)
  if(msg.objectID == robot.GetCarryingObject() ||
     msg.objectID == robot.GetDockObject())
  {
    const bool considerReaction = false;
    HandleNewObservation(msg.objectID, obsPose, msg.timestamp, considerReaction);
  }
  else {
    HandleNewObservation(msg.objectID, obsPose, msg.timestamp);
  }
}

bool BehaviorAcknowledgeObject::ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event,
                                                  const Robot& robot)
{
  if( event.GetTag() != EngineToGameTag::RobotObservedObject ) {
    PRINT_NAMED_ERROR("BehaviorAcknowledgeObject.ShouldRunForEvent.InvalidTag",
                      "Received trigger event with unhandled tag %hhu",
                      event.GetTag());
    return false;
  }

  const ExternalInterface::RobotObservedObject& msg = event.Get_RobotObservedObject();
  
  // NOTE: this may get called twice for this message, that should be OK. Call it here to ensure it is done
  // before returning
  HandleObjectObserved(robot, msg);

  // this will be set in begin iteration
  _currTarget.UnSet();
  return HasDesiredReactionTargets(robot);
} // ShouldRunForEvent()

  
void BehaviorAcknowledgeObject::HandleObjectMarkedUnknown(const Robot& robot, ObjectID objectID)
{
  const bool objectRemoved = RemoveReactionData(objectID);
  if(objectRemoved)
  {
    PRINT_NAMED_DEBUG("BehaviorAcknowledgeObject.HandleObjectMarkedUnknown",
                      "Removing Object %d from reacted set because it was marked unknown",
                      objectID.GetValue());
  }
} // HandleObjectMarkedUnknown()

} // namespace Cozmo
} // namespace Anki

