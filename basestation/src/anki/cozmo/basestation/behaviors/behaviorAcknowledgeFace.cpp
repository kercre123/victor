/**
 * File: behaviorAcknowledgeFace.cpp
 *
 * Author:  Andrew Stein
 * Created: 2016-06-16
 *
 * Description:  Simple quick reaction to a "new" face, just to show Cozmo has noticed you.
 *               Cozmo just turns towards the face and then plays a reaction animation.

 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorAcknowledgeFace.h"

#include "anki/common/basestation/utils/timer.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "anki/cozmo/basestation/actions/visuallyVerifyActions.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageFromActiveObject.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {
  
BehaviorAcknowledgeFace::BehaviorAcknowledgeFace(Robot& robot, const Json::Value& config)
: IBehaviorPoseBasedAcknowledgement(robot, config)
{
  SetDefaultName("AcknowledgeFace");

  SubscribeToTags({
    EngineToGameTag::RobotDeletedFace,
  });

  SubscribeToTriggerTags({
    EngineToGameTag::RobotObservedFace,
  });  
}
  
  
Result BehaviorAcknowledgeFace::InitInternalReactionary(Robot& robot)
{
  TurnTowardsPoseAction* turnAction = new TurnTowardsFaceAction(robot, _targetFace, _params.maxTurnAngle_rad);
  turnAction->SetTiltTolerance(_params.tiltTolerance_rad);
  turnAction->SetPanTolerance(_params.panTolerance_rad);

  CompoundActionSequential* action = new CompoundActionSequential(robot);
  action->AddAction(turnAction);

  if( _params.numImagesToWaitFor > 0 ) {
    // Will fail the action if we don't see the face, so we don't play the reaction
    // not looking at a face
    VisuallyVerifyFaceAction* verifyAction = new VisuallyVerifyFaceAction(robot, _targetFace);
    verifyAction->SetNumImagesToWaitFor(_params.numImagesToWaitFor);
    action->AddAction(verifyAction);
  }

  action->AddAction(new TriggerAnimationAction(robot, _params.reactionAnimTrigger));
  
  StartActing(action,
              [this, &robot](ActionResult res) {
                // Whether or not we succeeded, unset the target face (We've already added it to the reacted
                // set). Note that if we get interrupted by another reactionary behavior, this will not run
                RobotReactedToId(robot, _targetFace);
                _targetFace = Vision::UnknownFaceID;
              });

  
  return Result::RESULT_OK;
  
} // InitInternalReactionary()

  
bool BehaviorAcknowledgeFace::IsRunnableInternalReactionary(const Robot& robot) const
{
  return true; // TODO: consider !carrying object
}
  

void BehaviorAcknowledgeFace::AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotObservedFace:
      HandleFaceObserved(robot, event.GetData().Get_RobotObservedFace());
      break;
      
    case EngineToGameTag::RobotDeletedFace:
      HandleFaceDeleted(robot, event.GetData().Get_RobotDeletedFace().faceID);
      break;

    default:
      PRINT_NAMED_ERROR("BehaviorAcknowledgeFace.HandleWhileNotRunning.InvalidTag",
                        "Received event with unhandled tag %hhu.",
                        event.GetData().GetTag());
      break;
  }
} // AlwaysHandle()


void BehaviorAcknowledgeFace::HandleFaceObserved(const Robot& robot, const ExternalInterface::RobotObservedFace& msg)
{
  if( msg.faceID < 0 ) {
    // ignore temporary tracking-only ids
    return;
  }

  Pose3d facePose( msg.pose, robot.GetPoseOriginList() );
  HandleNewObservation(msg.faceID, facePose, msg.timestamp);
}

bool BehaviorAcknowledgeFace::ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event, const Robot& robot)
{
  if( event.GetTag() != EngineToGameTag::RobotObservedFace ) {
    PRINT_NAMED_ERROR("BehaviorAcknowledgeFace.ShouldRunForEvent.InvalidTag",
                      "Received trigger event with unhandled tag %hhu",
                      event.GetTag());
    return false;
  }
  
  std::set<s32> facesToReactTo;
  GetDesiredReactionTargets(robot, facesToReactTo);

  if( facesToReactTo.empty() ) {
    return false;
  }
  else {
    // for now, just react to one arbitrary face in the bunch.
    // TODO:(bn) properly handle multiple faces, like we are doing for objects
    _targetFace = *facesToReactTo.begin();
    return true;
  }
}
  
void BehaviorAcknowledgeFace::HandleFaceDeleted(const Robot& robot, Vision::FaceID_t faceID)
{
  if( faceID == _targetFace ) {
    _targetFace = Vision::UnknownFaceID;
  }
  
  const bool faceRemoved = RemoveReactionData(faceID);
  if(faceRemoved) {
    PRINT_NAMED_DEBUG("BehaviorAcknowledgeFace.HandleFaceDeleted",
                      "Removing Face %d from reacted set because it was deleted",
                      faceID);
  }
}


} // namespace Cozmo
} // namespace Anki

