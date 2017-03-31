/**
 * File: reactionTriggerStrategyPoseDifference.cpp
 *
 * Author: Andrew Stein :: Kevin M. Karol
 * Created: 2016-06-16 :: 12/08/16
 *
 * Description: Reaction Trigger strategy for responding to an object moving more than
 *
 * Copyright: Anki, Inc. 2016
 *
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyPositionUpdate.h"

#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

namespace {
CONSOLE_VAR(bool, kDebugAcknowledgements, "AcknowledgementBehaviors", false);
CONSOLE_VAR(f32, kHeadAngleDistFactor, "AcknowledgementBehaviors", 1.0);
CONSOLE_VAR(f32, kBodyAngleDistFactor, "AcknowledgementBehaviors", 3.0);

// static const char * const kCoolDownDurationKey    = "CoolDownDuration_ms";
// static const char * const kSamePoseDistKey        = "SamePoseDistThresh_mm";
// static const char * const kSamePoseDistSparkedKey = "SamePoseDistThresh_sparked_mm";
// static const char * const kSamePoseAngleKey       = "SamePoseAngleThresh_deg";
// no longer used (see COZMO-9862)

}

void ReactionTriggerStrategyPositionUpdate::ReactionData::FakeReaction()
{
  lastReactionPose = lastPose;
  lastReactionTime_ms = lastSeenTime_ms;
}


ReactionTriggerStrategyPositionUpdate::ReactionTriggerStrategyPositionUpdate(
                                          Robot& robot, const Json::Value& config,
                                          const std::string& strategyName,
                                          ReactionTrigger triggerAssociated)
: super(robot, config, strategyName)
, _triggerAssociated(triggerAssociated)
{
  
  SubscribeToTags({
    EngineToGameTag::RobotDelocalized
  });
}


// void ReactionTriggerStrategyPositionUpdate::LoadConfig(const Json::Value& config)
// {
//   using namespace JsonTools;
  
//   if(GetValueOptional(config, kCoolDownDurationKey , _params.coolDownDuration_ms)) {
//     PRINT_CH_DEBUG("ReactionTriggers","IBehaviorPoseBasedAcknowledgement.LoadConfig.SetCoolDownDuration",
//                       "%ums", _params.coolDownDuration_ms);
//   }
  
//   if(GetValueOptional(config, kSamePoseDistKey , _params.samePoseDistThreshold_mm)) {
//     PRINT_CH_DEBUG("ReactionTriggers","IBehaviorPoseBasedAcknowledgement.LoadConfig.SetPoseDistThresh",
//                       "%.1f", _params.samePoseDistThreshold_mm);
//   }
  
//   if(GetValueOptional(config, kSamePoseDistSparkedKey, _params.samePoseDistThreshold_sparked_mm)) {
//     PRINT_CH_DEBUG("ReactionTriggers","IBehaviorPoseBasedAcknowledgement.LoadConfig.SetPoseDistThreshSparked",
//                       "%.1f", _params.samePoseDistThreshold_sparked_mm);
//   }
  
  
//   if(GetAngleOptional(config, kSamePoseAngleKey, _params.samePoseAngleThreshold_rad, true)) {
//     PRINT_CH_DEBUG("ReactionTriggers","IBehaviorPoseBasedAcknowledgement.LoadConfig.SetPoseAngleThresh",
//                       "%.1fdeg", _params.samePoseAngleThreshold_rad.getDegrees());
//   }
// }
// No longer used (see COZMO-9862)


void ReactionTriggerStrategyPositionUpdate::AlwaysHandle(const EngineToGameEvent& event, const Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotDelocalized:
    {
      ResetReactionData();
      break;
    }
    
    default:
    break;
  }
  
  AlwaysHandlePoseBasedInternal(event, robot);
}

void ReactionTriggerStrategyPositionUpdate::AddReactionData(s32 idToAdd, ReactionData &&data)
{
  _reactionData[idToAdd] = std::move(data);
}

bool ReactionTriggerStrategyPositionUpdate::RemoveReactionData(s32 idToRemove)
{
  const size_t N = _reactionData.erase(idToRemove);
  const bool idRemoved = (N > 0);
  return idRemoved;
}

void ReactionTriggerStrategyPositionUpdate::HandleNewObservation(const Robot& robot,
                                                                 s32 id,
                                                                 const Pose3d& pose,
                                                                 u32 timestamp)
{
  const bool reactionEnabled = robot.GetBehaviorManager().
                     IsReactionTriggerEnabled(_triggerAssociated);
  HandleNewObservation(id, pose, timestamp, reactionEnabled);
}

void ReactionTriggerStrategyPositionUpdate::HandleNewObservation(s32 id,
                                                                 const Pose3d& pose,
                                                                 u32 timestamp,
                                                                 bool reactionEnabled)
{
  auto reactionIt = _reactionData.find(id);
  
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
      PRINT_CH_INFO("ReactionTriggers", (GetName() + ".AddNewID").c_str(),
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

bool ReactionTriggerStrategyPositionUpdate::ShouldReactToTarget_poseHelper(const Pose3d& thisPose,
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

bool ReactionTriggerStrategyPositionUpdate::ShouldReactToTarget(const Robot& robot,
                                                            const ReactionDataMap::value_type& reactionPair,
                                                            bool matchAnyPose) const
{
  // we should react unless we find a reason not to
  bool shouldReact = true;
  
  if( matchAnyPose ) {
    
    if( kDebugAcknowledgements ) {
      reactionPair.second.lastPose.Print("Behaviors", (GetName() + ".lastPose"));
    }
    
    // in this case, we need to check all poses regardless of ID
    for( const auto& otherPair : _reactionData ) {
      
      if( otherPair.second.lastReactionTime_ms == 0 ) {
        // don't match against something we've never reacted to (could be a brand new observation)
        if( kDebugAcknowledgements ) {
          PRINT_CH_INFO("ReactionTriggers", (GetName() + ".CheckAnyPose.Skip").c_str(),
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
        PRINT_CH_INFO("ReactionTriggers", (GetName() + ".CheckAnyPose").c_str(),
                      "%3d vs %3d: shouldReactToOther?%d ",
                      reactionPair.first,
                      otherPair.first,
                      shouldReactToOther ? 1 : 0);
        
        otherPair.second.lastReactionPose.Print("Behaviors", (GetName() + ".other.lastReaction"));
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
      const u32 currTimestamp = robot.GetLastImageTimeStamp();
      const bool isCooldownOver = currTimestamp - reactionPair.second.lastReactionTime_ms > _params.coolDownDuration_ms;
      
      const bool shouldReactToPose = ShouldReactToTarget_poseHelper(reactionPair.second.lastPose,
                                                                    reactionPair.second.lastReactionPose);
      shouldReact = isCooldownOver || shouldReactToPose;
      
      if( kDebugAcknowledgements ) {
        PRINT_CH_INFO("ReactionTriggers", (GetName() + ".SingleReaction").c_str(),
                      "%3d: shouldReact?%d isCooldownOver?%d shouldReactToPose?%d",
                      reactionPair.first,
                      shouldReact ? 1 : 0,
                      shouldReactToPose ? 1 : 0,
                      isCooldownOver ? 1 : 0);
        
        reactionPair.second.lastPose.Print("Behaviors", (GetName() + ".lastPose"));
        reactionPair.second.lastReactionPose.Print("Behaviors", (GetName() + ".lastReactionPose"));
      }
      
    }
    else {
      // else, we have never reacted to this ID, so do so now
      if( kDebugAcknowledgements ) {
        PRINT_CH_INFO("ReactionTriggers", (GetName() + ".DoInitialReaction").c_str(),
                      "Doing first reaction to new id %d at ts=%dms",
                      reactionPair.first,
                      reactionPair.second.lastSeenTime_ms);
        reactionPair.second.lastPose.Print("Behaviors", (GetName() + ".NewPose"));
      }
    }
  }
  
  return shouldReact;
}

void ReactionTriggerStrategyPositionUpdate::GetDesiredReactionTargets(const Robot& robot,
                                                                  std::set<s32>& targets,
                                                                  bool matchAnyPose) const
{
  // for each id we are tracking, check if it should be reacted to by comparing it's last observation (time
  // and pose) to the last time / place we reacted to it
  for( const auto& reactionPair : _reactionData ) {
    if( ShouldReactToTarget(robot, reactionPair, matchAnyPose) ) {
      targets.insert(reactionPair.first);
    }
  }
}

bool ReactionTriggerStrategyPositionUpdate::HasDesiredReactionTargets(const Robot& robot, bool matchAnyPose) const
{
  for( const auto& reactionPair : _reactionData ) {
    if( ShouldReactToTarget(robot, reactionPair, matchAnyPose) ) {
      return true;
    }
  }
  return false;
}

bool ReactionTriggerStrategyPositionUpdate::GetBestTarget(const Robot& robot, s32& bestTarget, bool matchAnyPose) const
{
  Pose3d poseWrtRobot;
  return GetBestTarget(robot, bestTarget, poseWrtRobot, matchAnyPose);
}

bool ReactionTriggerStrategyPositionUpdate::GetBestTarget(const Robot& robot, s32& bestTarget, Pose3d& poseWrtRobot, bool matchAnyPose) const
{
  // TODO:(bn) cache targets instead of doing this per-tick?
  std::set<s32> targets;
  GetDesiredReactionTargets(robot, targets, matchAnyPose);
  if (targets.empty()) {
    return false;
  }
  
  if( targets.size() == 1 ) {
    bestTarget = *targets.begin();
    
    auto reactionDataIter = _reactionData.find(bestTarget);
    DEV_ASSERT(reactionDataIter != _reactionData.end(), "ReactionTriggerStrategyPoseDifference.BadBestTargetId");
    
    if (false == reactionDataIter->second.lastPose.GetWithRespectTo(robot.GetPose(), poseWrtRobot))
    {
      // no transform, probably a different origin
      return false;
    }
    
    PRINT_CH_DEBUG("ReactionTriggers",(GetName() + ".GetBestTarget.SinglePose").c_str(),
                      "returning the only valid target id: %d",
                      bestTarget);
    return true;
  }
  
  float bestCost = std::numeric_limits<float>::max();
  bool ret = false;
  for( auto targetID : targets ) {
    DEV_ASSERT(_reactionData.find(targetID) != _reactionData.end(), "ReactionTriggerStrategyPoseDifference.BadTargetId");
    
    if( ! _reactionData.at(targetID).lastPose.GetWithRespectTo(robot.GetPose(), poseWrtRobot) ) {
      // no transform, probably a different origin
      continue;
    }
    
    Radians absHeadTurnAngle = TurnTowardsPoseAction::GetAbsoluteHeadAngleToLookAtPose(poseWrtRobot.GetTranslation());
    Radians relBodyTurnAngle = TurnTowardsPoseAction::GetRelativeBodyAngleToLookAtPose(poseWrtRobot.GetTranslation());
    
    Radians relHeadTurnAngle = absHeadTurnAngle - robot.GetHeadAngle();
    
    float cost = kHeadAngleDistFactor * relHeadTurnAngle.getAbsoluteVal().ToFloat() +
    kBodyAngleDistFactor * relBodyTurnAngle.getAbsoluteVal().ToFloat();
    
    PRINT_CH_DEBUG("ReactionTriggers",(GetName() + ".GetBestTarget.ConsiderPose").c_str(),
                      "pose id %d turns head by %fdeg, body by %fdeg, cost=%f",
                      targetID,
                      relHeadTurnAngle.getDegrees(),
                      relBodyTurnAngle.getDegrees(),
                      cost);
    if( cost < bestCost ) {
      bestTarget = targetID;
      bestCost = cost;
      ret = true;
    }
  }
  
  return ret;
}

void ReactionTriggerStrategyPositionUpdate::RobotReactedToId(const Robot& robot, s32 id)
{
  u32 currTimestamp = robot.GetLastImageTimeStamp();
  
  auto it = _reactionData.find(id);
  if( it != _reactionData.end() ) {
    it->second.lastReactionPose = it->second.lastPose; // this is the last observed pose
    it->second.lastReactionTime_ms = currTimestamp;
  }
  else {
    PRINT_CH_DEBUG("ReactionTriggers","ReactionTriggerStrategyPoseDifference.ReactionIdInvalid",
                      "robot reported that it finished reaction to id %d, but that doesn't exist, may have been deleted",
                      id);
  }
}

void ReactionTriggerStrategyPositionUpdate::ResetReactionData()
{
  for(auto reaction : _reactionData){
    reaction.second.lastReactionTime_ms = 0;
  }
}
  


} // namespace Cozmo
} // namespace Anki
