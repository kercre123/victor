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

#include "anki/cozmo/basestation/behaviors/reactionary/behaviorAcknowledgeFace.h"

#include "anki/common/basestation/utils/timer.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/visuallyVerifyActions.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageFromActiveObject.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

namespace AcknowledgeFaceConsoleVars{
CONSOLE_VAR(bool, kEnableFaceAcknowledgeReact, "AcknowledgeFaceBehavior", true);
CONSOLE_VAR(bool, kDebugFaceDist, "AcknowledgementBehaviors", false);
CONSOLE_VAR(u32, kNumImagesToWaitFor, "AcknowledgementBehaviors", 3);
CONSOLE_VAR_RANGED(f32, kDistanceToConsiderClose_mm, "AcknowledgementBehaviors", 300.0f, 0.0f, 1000.0f);
CONSOLE_VAR_RANGED(f32, kDistanceToConsiderClose_gap_mm, "AcknowledgementBehaviors", 100.0f, 0.0f, 1000.0f);
CONSOLE_VAR_RANGED(f32, kFaceReactCooldown_s, "AcknowledgementBehaviors", 4.0f, 0.0f, 60.0f);
CONSOLE_VAR(f32, kMaxTimeForInitialGreeting_s, "AcknowledgementBehaviors", 60.0f);
}

using namespace AcknowledgeFaceConsoleVars;
  
BehaviorAcknowledgeFace::BehaviorAcknowledgeFace(Robot& robot, const Json::Value& config)
: super(robot, config)
{
  SetDefaultName("AcknowledgeFace");

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

void BehaviorAcknowledgeFace::StopInternalReactionary(Robot& robot)
{
  _desiredTargets.clear();
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

bool BehaviorAcknowledgeFace::GetBestTarget(const Robot& robot)
{
  const AIWhiteboard& whiteboard = robot.GetAIComponent().GetWhiteboard();
  const bool preferName = false;  
  Vision::FaceID_t bestFace = whiteboard.GetBestFaceToTrack( _desiredTargets, preferName );
  
  if( bestFace == Vision::UnknownFaceID ) {
    return false;
  }
  else {
    _targetFace = bestFace;
    return true;
  }
}

void BehaviorAcknowledgeFace::BeginIteration(Robot& robot)
{
  _targetFace = Vision::UnknownFaceID;
  if( !GetBestTarget(robot) ) {
    return;
  }

  const bool sayName = true;
  TurnTowardsFaceAction* turnAction = new TurnTowardsFaceAction(robot,
                                                                _targetFace,
                                                                M_PI_F,
                                                                sayName);

  const float freeplayStartedTime_s = robot.GetBehaviorManager().GetFirstTimeFreeplayStarted();    
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();  
  const bool withinMinSessionTime = freeplayStartedTime_s >= 0.0f &&
    (currTime_s - freeplayStartedTime_s) <= kMaxTimeForInitialGreeting_s;
  const bool alreadyTurnedTowards = robot.GetFaceWorld().HasTurnedTowardsFace(_targetFace);
  const bool shouldPlayInitialGreeting = !_hasPlayedInitialGreeting && withinMinSessionTime && !alreadyTurnedTowards;

  PRINT_CH_INFO("Behaviors", "AcknowledgeFace.DoAcknowledgement",
                "currTime = %f, alreadyTurned:%d, shouldPlayGreeting:%d",
                currTime_s,
                alreadyTurnedTowards ? 1 : 0,
                shouldPlayInitialGreeting ? 1 : 0);
  
  if( shouldPlayInitialGreeting ) {
    turnAction->SetSayNameTriggerCallback([this](const Robot& robot, Vision::FaceID_t faceID){
        // only play the initial greeting once, so if we are going to use it, mark that here
        _hasPlayedInitialGreeting = true;
        return AnimationTrigger::NamedFaceInitialGreeting;
      });      
  }
  else {
    turnAction->SetSayNameAnimationTrigger(AnimationTrigger::AcknowledgeFaceNamed);
  }
  
  // if it's not named, always play this one
  turnAction->SetNoNameAnimationTrigger(AnimationTrigger::AcknowledgeFaceUnnamed);
  
  turnAction->SetMaxFramesToWait(kNumImagesToWaitFor);

  StartActing(turnAction, &BehaviorAcknowledgeFace::FinishIteration);
} // InitInternalReactionary()

void BehaviorAcknowledgeFace::FinishIteration(Robot& robot)
{
  _desiredTargets.erase( _targetFace );
  _hasReactedToFace.insert( _targetFace );
  _lastReactionTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  BehaviorObjectiveAchieved(BehaviorObjective::ReactedAcknowledgedFace);
  // move on to the next target, if there is one
  BeginIteration(robot);
}

  
bool BehaviorAcknowledgeFace::IsRunnableInternalReactionary(const Robot& robot) const
{
  return kEnableFaceAcknowledgeReact && !robot.IsOnCharger() && !robot.IsOnChargerPlatform();
}
  

void BehaviorAcknowledgeFace::AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot)
{

  if( !IsRunning() && IsRunnable(robot) ) {
    // will be handled by ShouldRunForEvent
    return;
  }
  
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotObservedFace:
      HandleFaceObserved(robot, event.GetData().Get_RobotObservedFace());
      break;
      

    default:
      PRINT_NAMED_ERROR("BehaviorAcknowledgeFace.HandleMessages.InvalidTag",
                        "Received event with unhandled tag %hhu.",
                        event.GetData().GetTag());
      break;
  }
} // AlwaysHandle()

bool BehaviorAcknowledgeFace::AddDesiredFace(Vision::FaceID_t faceID)
{
  if( IsReactionEnabled() ) {
    auto res = _desiredTargets.insert(faceID);
    return res.second;
  }
  else {
    // we aren't allowed to react, so we don't want to react to this face later. Add it to the list of things
    // we've reacted to, but return false, since we won't react now
    _hasReactedToFace.insert(faceID);
    return false;
  }
}

void BehaviorAcknowledgeFace::HandleFaceObserved(const Robot& robot, const ExternalInterface::RobotObservedFace& msg)
{
  // NOTE: this might get called twice for the same event (once from Handle and once from ShouldRunForEvent)

  if( msg.faceID < 0 ) {
    // ignore temporary tracking-only ids
    return;
  }

  const Vision::TrackedFace* face = robot.GetFaceWorld().GetFace(msg.faceID);
  if( nullptr == face ) {
    return;
  }

  // We always want to react the first time we see a named face
  const bool newNamedFace = face->HasName() && _hasReactedToFace.find( msg.faceID ) == _hasReactedToFace.end();

  if( newNamedFace ) {
    bool added = AddDesiredFace( msg.faceID );
    if(added) {
      PRINT_NAMED_DEBUG("BehaviorAcknowledgeFace.InitialFaceReaction",
                        "saw face ID %d (which is named) for the first time, want to react",
                        msg.faceID);
    }
  }

  float currDistance_mm = 0.0f;

  // We also want to react if the face was previously seen far away, but is now seen closer up
  if( ! ComputeDistanceBetween( robot.GetPose(), face->GetHeadPose(), currDistance_mm ) ) {
    PRINT_NAMED_ERROR("BehaviorAcknowledgeFace.PoseInWrongFrame",
                      "We couldnt get the distance from the robot to the face pose that we just saw...");
    robot.GetPose().Print("Unfiltered", "RobotPose");
    face->GetHeadPose().Print("Unfiltered", "HeadPose");
    return;
  }
  
  if( kDebugFaceDist ) {
    PRINT_CH_INFO("Behaviors", "BehaviorAcknowledgeFace.Debug.FaceDist",
                  "%d: %fmm",
                  msg.faceID,
                  currDistance_mm);
  }


  float closeThresh_mm = kDistanceToConsiderClose_mm;
  bool lastPoseClose = true; // (default to true so we don't react)
  
  auto it = _faceWasClose.find( msg.faceID );
  if( it != _faceWasClose.end() ) {
    lastPoseClose = it->second;
    if( lastPoseClose ) {
      // apply hysteresis to prevent flipping between close and not close
      closeThresh_mm += kDistanceToConsiderClose_gap_mm;
    }
  }

  const bool currPoseClose = currDistance_mm < closeThresh_mm;
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  const bool faceBecameClose = !lastPoseClose && currPoseClose;
  const bool notOnCooldown = _lastReactionTime_s < 0.0f || _lastReactionTime_s + kFaceReactCooldown_s <= currTime_s;
  if( faceBecameClose && notOnCooldown && ! newNamedFace ) {

    bool added = AddDesiredFace( msg.faceID );
    if(added) {
      PRINT_NAMED_DEBUG("BehaviorAcknowledgeFace.FaceBecomeClose",
                        "face ID %d became close (currDist = %fmm)",
                        msg.faceID,
                        currDistance_mm);
    }
  }

  _faceWasClose[ msg.faceID ] = currPoseClose;
}

bool BehaviorAcknowledgeFace::ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event, const Robot& robot)
{
  if( event.GetTag() != EngineToGameTag::RobotObservedFace ) {
    PRINT_NAMED_ERROR("BehaviorAcknowledgeFace.ShouldRunForEvent.InvalidTag",
                      "Received trigger event with unhandled tag %hhu",
                      event.GetTag());
    return false;
  }

  HandleFaceObserved(robot, event.Get_RobotObservedFace());

  return !_desiredTargets.empty();
}

} // namespace Cozmo
} // namespace Anki

