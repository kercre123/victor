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

namespace AcknowledgeFaceConsoleVars{
CONSOLE_VAR(bool, kEnableFaceAcknowledgeReact, "AcknowledgeFaceBehavior", false);
}
  
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
  // don't actually init until the first Update call. This gives other messages that came in this tick a
  // chance to be processed, in case we see multiple faces in the same tick.
  _shouldStart = true;

  return Result::RESULT_OK;
}

IBehavior::Status BehaviorAcknowledgeFace::UpdateInternal(Robot& robot)
{
  if( _shouldStart ) {
    _shouldStart = false;
    // now figure out which object to react to
    BeginIteration(robot);
  }

  return super::UpdateInternal(robot);
}


void BehaviorAcknowledgeFace::BeginIteration(Robot& robot)
{
  _targetFace = Vision::UnknownFaceID;
  s32 bestTarget = 0;
  if( GetBestTarget(robot, bestTarget) ) {
    _targetFace = bestTarget;
  }
  else {
    return;
  }

  TurnTowardsFaceAction* turnAction = new TurnTowardsFaceAction(robot,
                                                                _targetFace,
                                                                _params.maxTurnAngle_rad,
                                                                true); // say name
  turnAction->SetTiltTolerance(_params.tiltTolerance_rad);
  turnAction->SetPanTolerance(_params.panTolerance_rad);
  turnAction->SetSayNameAnimationTrigger(AnimationTrigger::AcknowledgeFaceNamed);
  turnAction->SetNoNameAnimationTrigger(AnimationTrigger::AcknowledgeFaceUnnamed);
  turnAction->SetMaxFramesToWait(_params.numImagesToWaitFor);

  StartActing(turnAction, &BehaviorAcknowledgeFace::FinishIteration);
} // InitInternalReactionary()

void BehaviorAcknowledgeFace::FinishIteration(Robot& robot)
{
  // inform parent class that we completed a reaction
  RobotReactedToId(robot, _targetFace);

  // move on to the next target, if there is one
  BeginIteration(robot);
}

  
bool BehaviorAcknowledgeFace::IsRunnableInternalReactionary(const Robot& robot) const
{
  // TODO: consider !carrying object, or maybe lock lift track if we are carrying?
  return AcknowledgeFaceConsoleVars::kEnableFaceAcknowledgeReact && !robot.IsOnCharger() && !robot.IsOnChargerPlatform();
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
  if( !IsRunnableInternalReactionary(robot) ) {
    // this let's us react as soon as e.g. we come off the charger
    return;
  }
  
  if( msg.faceID < 0 ) {
    // ignore temporary tracking-only ids

    // TODO:(bn) should we still track / react to tracking-only IDs? Andrew says it's not important because
    // they only last a very short time, or hang around if the face is too far or non-frontal, so we don't
    // really need to react to those anyway
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

  // this will be set in begin iteration
  _targetFace = Vision::UnknownFaceID;
  return HasDesiredReactionTargets(robot);
}
  
void BehaviorAcknowledgeFace::HandleFaceDeleted(const Robot& robot, Vision::FaceID_t faceID)
{
  if( faceID == _targetFace ) {
    _targetFace = Vision::UnknownFaceID;
    // NOTE: doesn't stop this iteration, so the reaction will still finish, then we'll go to another face if
    // we have one
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

