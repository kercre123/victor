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

#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerStrategyFacePositionUpdated.h"

#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorManager.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorAcknowledgeFace.h"
#include "engine/faceWorld.h"
#include "anki/common/basestation/utils/timer.h"
#include "engine/robot.h"
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


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ReactionTriggerStrategyFacePositionUpdated::ReactionTriggerStrategyFacePositionUpdated(BehaviorExternalInterface& behaviorExternalInterface,
                                                                                       const Json::Value& config)
: ReactionTriggerStrategyPositionUpdate(behaviorExternalInterface, config,
                                        kTriggerStrategyName, ReactionTrigger::FacePositionUpdated)
{
  SubscribeToTags({
    EngineToGameTag::RobotObservedFace
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyFacePositionUpdated::SetupForceTriggerBehavior(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr behavior)
{
  std::shared_ptr<BehaviorAcknowledgeFace> directPtr;
  behaviorExternalInterface.GetBehaviorContainer().FindBehaviorByIDAndDowncast(behavior->GetID(),
                                                         BehaviorClass::AcknowledgeFace,
                                                         directPtr);
  
  if(!_desiredTargets.empty()){
    directPtr->SetFacesToAcknowledge(_desiredTargets);
  }
  else{
    std::set<Vision::FaceID_t> possibleFaces = behaviorExternalInterface.GetFaceWorld().GetFaceIDs();
    if (!possibleFaces.empty()){
      //  acknowledge even if it's empty?
      directPtr->SetFacesToAcknowledge(possibleFaces);
    }
    else
    {
      PRINT_NAMED_WARNING("ReactionTriggerStrategyFacePositionUpdated.SetupForceTriggerBehavior", "Tried to run behavior without any faces");
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ReactionTriggerStrategyFacePositionUpdated::ShouldTriggerBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr behavior)
{
  std::shared_ptr<BehaviorAcknowledgeFace> directPtr;
  behaviorExternalInterface.GetBehaviorContainer().FindBehaviorByIDAndDowncast(behavior->GetID(),
                                                         BehaviorClass::AcknowledgeFace,
                                                         directPtr);
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  if(!_desiredTargets.empty()){
    directPtr->SetFacesToAcknowledge(_desiredTargets);
    const bool robotOffCharger = !robot.IsOnChargerPlatform();
    return robotOffCharger &&
           kEnableFaceAcknowledgeReact &&
           behavior->WantsToBeActivated(behaviorExternalInterface);
  }
  
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyFacePositionUpdated::HandleFaceObserved(BehaviorExternalInterface& behaviorExternalInterface,
                                                                    const ExternalInterface::RobotObservedFace& msg)
{
  if( msg.faceID < 0 ) {
    // ignore temporary tracking-only ids
    return;
  }
  
  const Vision::TrackedFace* face = behaviorExternalInterface.GetFaceWorld().GetFace(msg.faceID);
  if( nullptr == face ) {
    return;
  }
  
  // We always want to react the first time we see a named face
  const bool newNamedFace = face->HasName() && _hasReactedToFace.find( msg.faceID ) == _hasReactedToFace.end();
  
  if( newNamedFace ) {
    bool added = AddDesiredFace(behaviorExternalInterface, msg.faceID);
    if(added) {
      PRINT_NAMED_DEBUG("BehaviorAcknowledgeFace.InitialFaceReaction",
                        "saw face ID %d (which is named) for the first time, want to react",
                        msg.faceID);
    }
  }
  
  float currDistance_mm = 0.0f;
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
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
    
    bool added = AddDesiredFace(behaviorExternalInterface, msg.faceID);
    if(added) {
      PRINT_NAMED_DEBUG("BehaviorAcknowledgeFace.FaceBecomeClose",
                        "face ID %d became close (currDist = %fmm)",
                        msg.faceID,
                        currDistance_mm);
    }
  }
  
  _faceWasClose[ msg.faceID ] = currPoseClose;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ReactionTriggerStrategyFacePositionUpdated::AddDesiredFace(BehaviorExternalInterface& behaviorExternalInterface, Vision::FaceID_t faceID)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  if(robot.GetBehaviorManager().IsReactionTriggerEnabled(ReactionTrigger::FacePositionUpdated)) {
    auto res = _desiredTargets.insert(faceID);
    return res.second;
  }
  else {
    return false;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyFacePositionUpdated::AlwaysHandlePoseBasedInternal(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotObservedFace:
    {
      HandleFaceObserved(behaviorExternalInterface, event.GetData().Get_RobotObservedFace());
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


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyFacePositionUpdated::BehaviorThatStrategyWillTriggerInternal(ICozmoBehaviorPtr behavior)
{
  behavior->AddListener(this);
}

/////////////
// Implement IReactToFaceListener
/////////////


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyFacePositionUpdated::FinishedReactingToFace(BehaviorExternalInterface& behaviorExternalInterface, Vision::FaceID_t faceID)
{
  RobotReactedToId(behaviorExternalInterface, faceID);
  _hasReactedToFace.insert( faceID );
  _lastReactionTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyFacePositionUpdated::ClearDesiredTargets()
{
  _desiredTargets.clear();
}

  
  
} // namespace Cozmo
} // namespace Anki
