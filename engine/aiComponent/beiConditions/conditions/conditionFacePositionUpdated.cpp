/**
* File: ConditionFacePositionUpdated.h
*
* Author:  Kevin M. Karol
* Created: 11/1/17
*
* Description: Strategy which monitors large changes in face position
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/beiConditions/conditions/conditionFacePositionUpdated.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/beiConditions/beiConditionMessageHelper.h"
#include "engine/cozmoContext.h"
#include "engine/faceWorld.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {
  
namespace{
const bool kDebugFaceDist = false;
CONSOLE_VAR_RANGED(f32, kDistanceToConsiderClose_mm, "AcknowledgementBehaviors", 300.0f, 0.0f, 1000.0f);
CONSOLE_VAR_RANGED(f32, kDistanceToConsiderClose_gap_mm, "AcknowledgementBehaviors", 100.0f, 0.0f, 1000.0f);
CONSOLE_VAR_RANGED(f32, kFaceReactCooldown_s, "AcknowledgementBehaviors", 4.0f, 0.0f, 60.0f);

}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionFacePositionUpdated::ConditionFacePositionUpdated(const Json::Value& config, BEIConditionFactory& factory)
  : IBEICondition(config, factory)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionFacePositionUpdated::~ConditionFacePositionUpdated()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionFacePositionUpdated::InitInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  _messageHelper.reset(new BEIConditionMessageHelper(this, behaviorExternalInterface));
  _messageHelper->SubscribeToTags({
    EngineToGameTag::RobotObservedFace
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionFacePositionUpdated::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{ 
  // add a check for offTreadsState?
  return !_desiredTargets.empty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionFacePositionUpdated::HandleEvent(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotObservedFace:
    {
      HandleFaceObserved(behaviorExternalInterface, event.GetData().Get_RobotObservedFace());
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
void ConditionFacePositionUpdated::HandleFaceObserved(BehaviorExternalInterface& behaviorExternalInterface,
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
  const auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
  // We also want to react if the face was previously seen far away, but is now seen closer up
  if( ! ComputeDistanceBetween( robotInfo.GetPose(), face->GetHeadPose(), currDistance_mm ) ) {
    PRINT_NAMED_ERROR("BehaviorAcknowledgeFace.PoseInWrongFrame",
                      "We couldnt get the distance from the robot to the face pose that we just saw...");
    robotInfo.GetPose().Print("Unfiltered", "RobotPose");
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
bool ConditionFacePositionUpdated::AddDesiredFace(BehaviorExternalInterface& behaviorExternalInterface, Vision::FaceID_t faceID)
{
  auto res = _desiredTargets.insert(faceID);
  return res.second;
}


} // namespace Cozmo
} // namespace Anki
