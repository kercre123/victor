/**
 * File: behaviorPoseBasedAcknowledgementInterface.cpp
 *
 * Author:  Andrew Stein
 * Created: 2016-06-16
 *
 * Description:  Base class for distraction behaviors

 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/behaviors/behaviorPoseBasedAcknowledgementInterface.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/cozmo/basestation/robot.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

namespace {
CONSOLE_VAR(bool, kDebugAcknowledgements, "AcknowledgementBehaviors", false);
CONSOLE_VAR(f32, kHeadAngleDistFactor, "AcknowledgementBehaviors", 1.0);
CONSOLE_VAR(f32, kBodyAngleDistFactor, "AcknowledgementBehaviors", 3.0);
}

static const char * const kReactionAnimGroupKey = "ReactionAnimGroup";
static const char * const kMaxTurnAngleKey      = "MaxTurnAngle_deg";
static const char * const kCoolDownDurationKey  = "CoolDownDuration_ms";
static const char * const kSamePoseDistKey      = "SamePoseDistThresh_mm";
static const char * const kSamePoseAngleKey     = "SamePoseAngleThresh_deg";
static const char * const kPanToleranceKey      = "PanTolerance_deg";
static const char * const kTiltToleranceKey     = "TiltTolerance_deg";
static const char * const kNumImagesToWaitForKey= "NumImagesToWaitFor";

void IBehaviorPoseBasedAcknowledgement::ReactionData::FakeReaction()
{
  lastReactionPose = lastPose;
  lastReactionTime_ms = lastSeenTime_ms;
}

IBehaviorPoseBasedAcknowledgement::IBehaviorPoseBasedAcknowledgement(Robot& robot, const Json::Value& config)
: super(robot, config)
{
  LoadConfig(config);
}

void IBehaviorPoseBasedAcknowledgement::LoadConfig(const Json::Value& config)
{
  using namespace JsonTools;
  
  JsonTools::GetValueOptional(config,kReactionAnimGroupKey,_params.reactionAnimTrigger);
  
  if(GetAngleOptional(config, kMaxTurnAngleKey, _params.maxTurnAngle_rad, true)) {
    PRINT_NAMED_DEBUG("IBehaviorPoseBasedAcknowledgement.LoadConfig.SetMaxTurnAngle",
                      "%.1fdeg", _params.maxTurnAngle_rad.getDegrees());
  }
  
  if(GetValueOptional(config, kCoolDownDurationKey , _params.coolDownDuration_ms)) {
    PRINT_NAMED_DEBUG("IBehaviorPoseBasedAcknowledgement.LoadConfig.SetCoolDownDuration",
                      "%ums", _params.coolDownDuration_ms);
  }
  
  if(GetValueOptional(config, kSamePoseDistKey , _params.samePoseDistThreshold_mm)) {
    PRINT_NAMED_DEBUG("IBehaviorPoseBasedAcknowledgement.LoadConfig.SetPoseDistThresh",
                      "%.1f", _params.samePoseDistThreshold_mm);
  }
  
  if(GetAngleOptional(config, kSamePoseAngleKey, _params.samePoseAngleThreshold_rad, true)) {
    PRINT_NAMED_DEBUG("IBehaviorPoseBasedAcknowledgement.LoadConfig.SetPoseAngleThresh",
                      "%.1fdeg", _params.samePoseAngleThreshold_rad.getDegrees());
  }
  
  if(GetAngleOptional(config, kPanToleranceKey, _params.panTolerance_rad, true)) {
    PRINT_NAMED_DEBUG("IBehaviorPoseBasedAcknowledgement.LoadConfig.SetPanTolerance",
                      "%.1fdeg", _params.panTolerance_rad.getDegrees());
  }
  
  if(GetAngleOptional(config, kTiltToleranceKey, _params.tiltTolerance_rad, true)) {
    PRINT_NAMED_DEBUG("IBehaviorPoseBasedAcknowledgement.LoadConfig.SetTiltTolerance",
                      "%.1fdeg", _params.tiltTolerance_rad.getDegrees());
  }
  
  if(GetValueOptional(config, kNumImagesToWaitForKey, _params.numImagesToWaitFor)) {
    PRINT_NAMED_DEBUG("IBehaviorPoseBasedAcknowledgement.LoadConfig.SetNumImagesToWaitFor",
                      "%d", _params.numImagesToWaitFor);
  }
} // LoadConfig()
  

void IBehaviorPoseBasedAcknowledgement::StopInternalReactionary(Robot& robot)
{
  // if the behavior gets interrupted, pretend we finished reacting to every id just now (since we don't want
  // the to queue up while we are disabled or interrupted by another reaction)
  for( auto& reactionPair : _reactionData ) {
    reactionPair.second.lastReactionPose = reactionPair.second.lastPose;
    reactionPair.second.lastReactionTime_ms = robot.GetLastImageTimeStamp();
  }
  
  StopInternalAcknowledgement(robot);
}


void IBehaviorPoseBasedAcknowledgement::AddReactionData(s32 idToAdd, ReactionData &&data)
{
  _reactionData[idToAdd] = std::move(data);
}
  
bool IBehaviorPoseBasedAcknowledgement::RemoveReactionData(s32 idToRemove)
{
  const size_t N = _reactionData.erase(idToRemove);
  const bool idRemoved = (N > 0);
  return idRemoved;
}

void IBehaviorPoseBasedAcknowledgement::HandleNewObservation(s32 id, const Pose3d& pose, u32 timestamp)
{
  const bool reactionEnabled = IsReactionEnabled();
  HandleNewObservation(id, pose, timestamp, reactionEnabled);
}

void IBehaviorPoseBasedAcknowledgement::HandleNewObservation(s32 id,
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

bool IBehaviorPoseBasedAcknowledgement::ShouldReactToTarget(const Robot& robot,
                                                            const ReactionDataMap::value_type& reactionPair) const
{  
  u32 currTimestamp = robot.GetLastImageTimeStamp();
  
  bool shouldReact = reactionPair.second.lastReactionTime_ms == 0;
  // if we have never reacted, we always want to. Otherwise, check the cooldown and pose

  if( !shouldReact ) {
    const bool isCoolDownOver = currTimestamp - reactionPair.second.lastReactionTime_ms > _params.coolDownDuration_ms;
    const bool isPoseDifferent = !reactionPair.second.lastPose.IsSameAs(reactionPair.second.lastReactionPose,
                                                                        _params.samePoseDistThreshold_mm,
                                                                        _params.samePoseAngleThreshold_rad);
    shouldReact = isCoolDownOver || isPoseDifferent;

    if( kDebugAcknowledgements ) {
      std::string eventName;
      if( shouldReact ) {
        eventName = GetName() + ".HandleNewObservation.ShouldReactAgain";
      }
      else {
        eventName = GetName() + ".HandleNewObservation.DontReactAgain";
      }
        
      PRINT_CH_INFO("Behaviors", eventName.c_str(),
                    "%3d: cooldownOver?%d poseDifferent?%d lastSeenTime=%dms, currTime=%dms, (cooldown=%dms), "
                    "lastReactionPose=(%f, %f, %f) @time %dms, currPose=(%f, %f, %f)",
                    reactionPair.first,
                    isCoolDownOver ? 1 : 0,
                    isPoseDifferent ? 1 : 0,
                    reactionPair.second.lastSeenTime_ms,
                    currTimestamp,
                    _params.coolDownDuration_ms,
                    reactionPair.second.lastReactionPose.GetTranslation().x(),
                    reactionPair.second.lastReactionPose.GetTranslation().y(),
                    reactionPair.second.lastReactionPose.GetTranslation().z(),
                    reactionPair.second.lastReactionTime_ms,
                    reactionPair.second.lastPose.GetTranslation().x(),
                    reactionPair.second.lastPose.GetTranslation().y(),
                    reactionPair.second.lastPose.GetTranslation().z());
    }
  }
  else if( kDebugAcknowledgements ) {
    PRINT_CH_INFO("Behaviors", (GetName() + ".DoInitialReaction").c_str(),
                  "Doing first reaction to new id %d at ts=%dms, pose (%f, %f, %f)",
                  reactionPair.first,
                  reactionPair.second.lastSeenTime_ms,
                  reactionPair.second.lastPose.GetTranslation().x(),
                  reactionPair.second.lastPose.GetTranslation().y(),
                  reactionPair.second.lastPose.GetTranslation().z());
  }
                    
  return shouldReact;
}

void IBehaviorPoseBasedAcknowledgement::GetDesiredReactionTargets(const Robot& robot, std::set<s32>& targets) const
{
  // for each id we are tracking, check if it should be reacted to by comparing it's last observation (time
  // and pose) to the last time / place we reacted to it
  for( const auto& reactionPair : _reactionData ) {
    if( ShouldReactToTarget(robot, reactionPair) ) {
      targets.insert(reactionPair.first);
    }
  }
}

bool IBehaviorPoseBasedAcknowledgement::HasDesiredReactionTargets(const Robot& robot) const
{
  for( const auto& reactionPair : _reactionData ) {
    if( ShouldReactToTarget(robot, reactionPair) ) {
      return true;
    }
  }
  return false;
}

bool IBehaviorPoseBasedAcknowledgement::GetBestTarget(const Robot& robot, s32& bestTarget) const
{
  // TODO:(bn) cache targets instead of doing this per-tick?
  std::set<s32> targets;
  GetDesiredReactionTargets(robot, targets);
  if( targets.empty() ) {
    return false;
  }

  if( targets.size() == 1 ) {
    bestTarget = *targets.begin();
    PRINT_NAMED_DEBUG((GetName() + ".GetBestTarget.SinglePose").c_str(),
                      "returning the only valid target id: %d",
                      bestTarget);
    return true;
  }

  float bestCost = std::numeric_limits<float>::max();
  bool ret = false;
  for( auto targetID : targets ) {
    ASSERT_NAMED(_reactionData.find(targetID) != _reactionData.end(), "IBehaviorPoseBasedAcknowledgement.BadTargetId");
    
    Pose3d poseWrtRobot;
    if( ! _reactionData.at(targetID).lastPose.GetWithRespectTo(robot.GetPose(), poseWrtRobot) ) {
      // no transform, probably a different origin
      continue;
    }

    Radians absHeadTurnAngle = TurnTowardsPoseAction::GetAbsoluteHeadAngleToLookAtPose(poseWrtRobot.GetTranslation());
    Radians relBodyTurnAngle = TurnTowardsPoseAction::GetRelativeBodyAngleToLookAtPose(poseWrtRobot.GetTranslation());

    Radians relHeadTurnAngle = absHeadTurnAngle - robot.GetHeadAngle();

    float cost = kHeadAngleDistFactor * relHeadTurnAngle.getAbsoluteVal().ToFloat() +
      kBodyAngleDistFactor * relBodyTurnAngle.getAbsoluteVal().ToFloat();

    PRINT_NAMED_DEBUG((GetName() + ".GetBestTarget.ConsiderPose").c_str(),
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

void IBehaviorPoseBasedAcknowledgement::RobotReactedToId(const Robot& robot, s32 id)
{
  u32 currTimestamp = robot.GetLastImageTimeStamp();

  auto it = _reactionData.find(id);
  if( it != _reactionData.end() ) {
    it->second.lastReactionPose = it->second.lastPose; // this is the last observed pose
    it->second.lastReactionTime_ms = currTimestamp;
  }
  else {
    PRINT_NAMED_DEBUG("IBehaviorPoseBasedAcknowledgement.ReactionIdInvalid",
                      "robot reported that it finished reaction to id %d, but that doesn't exist, may have been deleted",
                      id);
  }
}



} // namespace Cozmo
} // namespace Anki

