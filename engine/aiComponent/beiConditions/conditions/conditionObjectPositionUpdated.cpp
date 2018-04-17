/**
* File: ConditionObjectPositionUpdated.h
*
* Author: Matt Michini - Kevin M. Karol
* Created: 2017/01/11  - 7/5/17
*
* Description: Strategy for responding to an object that's moved sufficiently far
* in block world
*
* Copyright: Anki, Inc. 2017
*
*
**/

#include "engine/aiComponent/beiConditions/conditions/conditionObjectPositionUpdated.h"

#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/beiConditions/beiConditionMessageHelper.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/dockingComponent.h"
#include "engine/cozmoContext.h"

#include "coretech/common/engine/math/point_impl.h"


namespace Anki {
namespace Cozmo {

namespace{
std::set<ObjectFamily> _objectFamilies = {{
  ObjectFamily::LightCube,
  ObjectFamily::Block
}};
const bool kDebugAcknowledgements = false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionObjectPositionUpdated::ConditionObjectPositionUpdated(const Json::Value& config, BEIConditionFactory& factory)
  : IBEICondition(config, factory)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionObjectPositionUpdated::~ConditionObjectPositionUpdated()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionObjectPositionUpdated::InitInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  _messageHelper.reset(new BEIConditionMessageHelper(this, behaviorExternalInterface));
  _messageHelper->SubscribeToTags({
    EngineToGameTag::RobotObservedObject,
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionObjectPositionUpdated::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  // add a check for offTreadsState?
  return HasDesiredReactionTargets(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionObjectPositionUpdated::HandleEvent(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) 
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotObservedObject: {
      HandleObjectObserved(behaviorExternalInterface, event.GetData().Get_RobotObservedObject());
      break;
    }
    
    case EngineToGameTag::RobotDelocalized:
    {
      // this is passed through from the parent class - it's valid to be receiving this
      // we just currently don't use it for anything.  Case exists so we don't print errors.
      break;
    }
    
    default:
    PRINT_NAMED_ERROR("BehaviorAcknowledgeObject.HandleWhileNotRunning.InvalidTag",
                      "Received event with unhandled tag %hhu.",
                      event.GetData().GetTag());
    break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionObjectPositionUpdated::HandleObjectObserved(BehaviorExternalInterface& behaviorExternalInterface,
  const ExternalInterface::RobotObservedObject& msg)
{
  // Object must be in one of the families this behavior cares about
  const bool hasValidFamily = _objectFamilies.count(msg.objectFamily) > 0;
  if(!hasValidFamily) {
    return;
  }

  const auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
  // check if we want to react based on pose and cooldown, and also update position data even if we don't
  // react
  Pose3d obsPose( msg.pose, robotInfo.GetPoseOriginList() );

  // ignore cubes we are carrying or docking to (don't react to them)
  if(msg.objectID == robotInfo.GetCarryingComponent().GetCarryingObject() ||
     msg.objectID == robotInfo.GetDockingComponent().GetDockObject())
  {
    const bool considerReaction = false;
    HandleNewObservation(msg.objectID, obsPose, msg.timestamp, considerReaction);
  }
  else {
    HandleNewObservation(msg.objectID, obsPose, msg.timestamp);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionObjectPositionUpdated::AddReactionData(s32 idToAdd, ReactionData &&data)
{
  _reactionData[idToAdd] = std::move(data);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionObjectPositionUpdated::RemoveReactionData(s32 idToRemove)
{
  const size_t N = _reactionData.erase(idToRemove);
  const bool idRemoved = (N > 0);
  return idRemoved;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionObjectPositionUpdated::ReactionData::FakeReaction()
{
  lastReactionPose = lastPose;
  lastReactionTime_ms = lastSeenTime_ms;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionObjectPositionUpdated::HandleNewObservation(s32 id,
                                                         const Pose3d& pose,
                                                         u32 timestamp,
                                                         bool reactionEnabled)
{
  const auto reactionIt = _reactionData.find(id);
  
  if( reactionIt == _reactionData.end() ) {
    ReactionData reaction{
      .lastPose            = pose,
      .lastSeenTime_ms     = timestamp,
      .lastReactionTime_ms = 0,
    };

    // if the reactionary behavior is disabled, that means we don't want to react, so fake it to look like we
    // just reacted
    if( ! reactionEnabled ) {
      reaction.FakeReaction();
    }

    if( kDebugAcknowledgements ) {
      PRINT_CH_INFO("ReactionTriggers", ("ConditionObjectPositionUpdated.AddNewID"),
                    "%d seen for the first time at (%f, %f, %f) @time %dms reactionEnabled=%d",
                    id,
                    pose.GetTranslation().x(),
                    pose.GetTranslation().y(),
                    pose.GetTranslation().z(),
                    timestamp,
                    reactionEnabled ? 1 : 0);
    }
    
    AddReactionData(id, std::move(reaction));
  }
  else {
    reactionIt->second.lastPose = pose;
    reactionIt->second.lastSeenTime_ms = timestamp;

    if( ! reactionEnabled ) {
      // same as above, fake reacting it now, so it won't try to react once we re-enable
      reactionIt->second.FakeReaction();
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionObjectPositionUpdated::ShouldReactToTarget_poseHelper(const Pose3d& thisPose,
                                                                   const Pose3d& otherPose) const
{
  // TODO:(bn) ideally pose should have an IsSameAs function which can return "yes", "no", or
  // "don't know" / "wrong frame"
  
  Pose3d otherPoseWrtThis;
  if( ! otherPose.GetWithRespectTo(thisPose, otherPoseWrtThis) ) {
    // poses aren't in the same frame, so don't react (assume we moved, not the cube)
    return false;
  }
  
  float distThreshold = _params.samePoseDistThreshold_mm;
  
  const bool thisPoseIsSame = thisPose.IsSameAs(otherPoseWrtThis,
                                                distThreshold,
                                                _params.samePoseAngleThreshold_rad);
  
  // we want to react if the pose is not the same
  return !thisPoseIsSame;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionObjectPositionUpdated::ShouldReactToTarget(BehaviorExternalInterface& behaviorExternalInterface,
                                                            const ReactionDataMap::value_type& reactionPair,
                                                            bool matchAnyPose) const
{
  // we should react unless we find a reason not to
  bool shouldReact = true;
  
  if( matchAnyPose ) {
    
    if( kDebugAcknowledgements ) {
      reactionPair.second.lastPose.Print("Behaviors", ("ConditionObjectPositionUpdated.lastPose"));
    }
    
    // in this case, we need to check all poses regardless of ID
    for( const auto& otherPair : _reactionData ) {
      
      if( otherPair.second.lastReactionTime_ms == 0 ) {
        // don't match against something we've never reacted to (could be a brand new observation)
        if( kDebugAcknowledgements ) {
          PRINT_CH_INFO("ReactionTriggers", ("ConditionObjectPositionUpdated.CheckAnyPose.Skip"),
                        "%3d vs %3d: skip because haven't reacted",
                        reactionPair.first,
                        otherPair.first);
        }
        
        continue;
      }
      
      // if any single pose says we don't need to react, then don't react
      const bool shouldReactToOther = ShouldReactToTarget_poseHelper(reactionPair.second.lastPose,
                                                                     otherPair.second.lastReactionPose);
      
      if( kDebugAcknowledgements ) {
        PRINT_CH_INFO("ReactionTriggers", ("ConditionObjectPositionUpdated.CheckAnyPose"),
                      "%3d vs %3d: shouldReactToOther?%d ",
                      reactionPair.first,
                      otherPair.first,
                      shouldReactToOther ? 1 : 0);
        
        otherPair.second.lastReactionPose.Print("Behaviors", ("ConditionObjectPositionUpdated.other.lastReaction"));
      }
      
      if( !shouldReactToOther ) {
        shouldReact = false;
        break;
      }
    }
  }
  else {
    
    const bool hasEverReactedToThisId = reactionPair.second.lastReactionTime_ms > 0;
    
    // if we have reacted, then check if we want to react again based on the last pose and time we reacted.
    if( hasEverReactedToThisId ) {
      const auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
      const u32 currTimestamp = robotInfo.GetLastImageTimeStamp();
      const bool isCooldownOver = currTimestamp - reactionPair.second.lastReactionTime_ms > _params.coolDownDuration_ms;
      
      const bool shouldReactToPose = ShouldReactToTarget_poseHelper(reactionPair.second.lastPose,
                                                                    reactionPair.second.lastReactionPose);
      shouldReact = isCooldownOver || shouldReactToPose;
      
      if( kDebugAcknowledgements ) {
        PRINT_CH_INFO("ReactionTriggers", ("ConditionObjectPositionUpdated.SingleReaction"),
                      "%3d: shouldReact?%d isCooldownOver?%d shouldReactToPose?%d",
                      reactionPair.first,
                      shouldReact ? 1 : 0,
                      shouldReactToPose ? 1 : 0,
                      isCooldownOver ? 1 : 0);
        
        reactionPair.second.lastPose.Print("Behaviors", ("ConditionObjectPositionUpdated.lastPose"));
        reactionPair.second.lastReactionPose.Print("Behaviors", ("ConditionObjectPositionUpdated.lastReactionPose"));
      }
      
    }
    else {
      // else, we have never reacted to this ID, so do so now
      if( kDebugAcknowledgements ) {
        PRINT_CH_INFO("ReactionTriggers", ("ConditionObjectPositionUpdated.DoInitialReaction"),
                      "Doing first reaction to new id %d at ts=%dms",
                      reactionPair.first,
                      reactionPair.second.lastSeenTime_ms);
        reactionPair.second.lastPose.Print("Behaviors", ("ConditionObjectPositionUpdated.NewPose"));
      }
    }
  }
  
  return shouldReact;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionObjectPositionUpdated::HasDesiredReactionTargets(BehaviorExternalInterface& behaviorExternalInterface, bool matchAnyPose) const
{
  for( auto& reactionPair : _reactionData ) {
    if( ShouldReactToTarget(behaviorExternalInterface, reactionPair, matchAnyPose) ) {
      reactionPair.second.FakeReaction();
      return true;
    }
  }
  return false;
}


} // namespace Cozmo
} // namespace Anki
