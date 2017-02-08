/**
 * File: reactionTriggerStrategyFace.cpp
 *
 * Author: Andrew Stein :: Kevin M. Karol
 * Created: 2016-06-16 :: 12/08/16
 *
 * Description: Reaction Trigger strategy for responding to
 *
 * Copyright: Anki, Inc. 2016
 *
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyFacePositionUpdated.h"

#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqAcknowledgeFace.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/vision/basestation/faceIdTypes.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

  
namespace{
CONSOLE_VAR(bool, kDebugFaceDist, "AcknowledgementBehaviors", false);
CONSOLE_VAR_RANGED(f32, kDistanceToConsiderClose_mm, "AcknowledgementBehaviors", 300.0f, 0.0f, 1000.0f);
CONSOLE_VAR_RANGED(f32, kDistanceToConsiderClose_gap_mm, "AcknowledgementBehaviors", 100.0f, 0.0f, 1000.0f);
CONSOLE_VAR_RANGED(f32, kFaceReactCooldown_s, "AcknowledgementBehaviors", 4.0f, 0.0f, 60.0f);
CONSOLE_VAR(bool, kEnableFaceAcknowledgeReact, "AcknowledgeFaceBehavior", true);
static const char* kTriggerStrategyName = "Strategy React To Face Position Updated";
}
  
ReactionTriggerStrategyFacePositionUpdated::ReactionTriggerStrategyFacePositionUpdated(Robot& robot, const Json::Value& config)
: ReactionTriggerStrategyPositionUpdate(robot, config, kTriggerStrategyName)
{
  SubscribeToTags({
    EngineToGameTag::RobotObservedFace
  });
}


bool ReactionTriggerStrategyFacePositionUpdated::ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior)
{
  if(!_desiredTargets.empty()){
    BehaviorPreReqAcknowledgeFace acknowledgeFacePreReqs(_desiredTargets);
    const bool robotOffCharger = !robot.IsOnChargerPlatform();
    return robotOffCharger && kEnableFaceAcknowledgeReact &&
            behavior->IsRunnable(acknowledgeFacePreReqs);
  }
  
  return false;
}


void ReactionTriggerStrategyFacePositionUpdated::HandleFaceObserved(const Robot& robot, const ExternalInterface::RobotObservedFace& msg)
{
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
    PRINT_CH_INFO("ReactionTriggers", "BehaviorAcknowledgeFace.Debug.FaceDist",
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

  
bool ReactionTriggerStrategyFacePositionUpdated::AddDesiredFace(Vision::FaceID_t faceID)
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

  
  
void ReactionTriggerStrategyFacePositionUpdated::AlwaysHandlePoseBasedInternal(const EngineToGameEvent& event, const Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotObservedFace:
    {
      HandleFaceObserved(robot, event.GetData().Get_RobotObservedFace());
      break;
    }
    
    case EngineToGameTag::RobotDelocalized:
    {
      // Fallthrough from parent class
      break;
    }
    
    default:
    {
      PRINT_NAMED_ERROR("ReactionStrategyFacePositionUpdate.HandleMessages.InvalidTag",
                      "Received event with unhandled tag %hhu.",
                      event.GetData().GetTag());
    }
    break;
  }
}

void ReactionTriggerStrategyFacePositionUpdated::BehaviorThatStrategyWillTrigger(IBehavior* behavior)
{
  behavior->AddListener(this);
}

/////////////
// Implement IReactToFaceListener
/////////////
  
void ReactionTriggerStrategyFacePositionUpdated::FinishedReactingToFace(Robot& robot, Vision::FaceID_t faceID)
{
  RobotReactedToId(robot, faceID);
  _hasReactedToFace.insert( faceID );
  _lastReactionTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}
  
void ReactionTriggerStrategyFacePositionUpdated::ClearDesiredTargets()
{
  _desiredTargets.clear();
}

  
  
} // namespace Cozmo
} // namespace Anki
