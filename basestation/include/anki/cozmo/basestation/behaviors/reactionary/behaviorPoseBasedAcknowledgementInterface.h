/**
 * File: behaviorPoseBasedAcknowledgementInterface.h
 *
 * Author:  Andrew Stein / Brad Neuman
 * Created: 2016-06-16
 *
 * Description: Base class for behaviors which sotp to acknowledge something at a given pose
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_IBehaviorPoseBasedAcknowledgement_H__
#define __Cozmo_Basestation_Behaviors_IBehaviorPoseBasedAcknowledgement_H__

#include "anki/common/basestation/math/pose.h"
#include "anki/common/shared/radians.h"

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "clad/types/animationTrigger.h"

#include <map>

namespace Anki {
namespace Cozmo {
  
class IBehaviorPoseBasedAcknowledgement : public IReactionaryBehavior
{
public:

  virtual bool ShouldResumeLastBehavior() const override { return true; }

  
protected:
  // Note: no public constructor, not directly instantiable, including by BehaviorFactory
  IBehaviorPoseBasedAcknowledgement(Robot& robot, const Json::Value& config);

  virtual void StopInternalReactionary(Robot& robot) final override;
  virtual void StopInternalAcknowledgement(Robot& robot) {};  
  
  // Default configuration parameters which can be overriden by JSON config
  struct ConfigParams {
    AnimationTrigger reactionAnimTrigger   = AnimationTrigger::Count;
    Radians     maxTurnAngle_rad           = DEG_TO_RAD_F32(45.f);
    Radians     samePoseAngleThreshold_rad = DEG_TO_RAD_F32(45.f);
    Radians     panTolerance_rad           = DEG_TO_RAD_F32(5.f);
    Radians     tiltTolerance_rad          = DEG_TO_RAD_F32(5.f);
    TimeStamp_t coolDownDuration_ms        = 600000; // time since last seen (in same pose) before re-reaction
    float       samePoseDistThreshold_mm   = 80.f;
    float       samePoseDistThreshold_sparked_mm   = 30.f;
    s32         numImagesToWaitFor         = 2; // After turning before playing animation, 0 means don't verify
  } _params;
  
  void LoadConfig(const Json::Value& config);
  
  struct ReactionData
  {
    Pose3d      lastPose;
    Pose3d      lastReactionPose;
    TimeStamp_t lastSeenTime_ms;
    TimeStamp_t lastReactionTime_ms;

    // helper function to fake a reaction (useful for cases where we want to ignore reactions)
    void FakeReaction();
  };

  void AddReactionData(s32 idToAdd, ReactionData&& data);
  bool RemoveReactionData(s32 idToRemove);

  // handles observing a new ID. Updates internal reaction data. If reactionEnabled is omitted, it checks
  // IsReactionEnabled(). If reactionEnabled is false, it will add the observation but ignore it for reactions
  // (by faking a reaction)
  void HandleNewObservation(s32 id, const Pose3d& pose, u32 timestamp);
  void HandleNewObservation(s32 id, const Pose3d& pose, u32 timestamp, bool reactionEnabled);

  // For the next three functions, the bool `matchAnyPose` defaults to false. If true, then it checks poses
  // against any other pose, regardless of ID. If false, it only checks against it's own ID. Cooldown is still
  // checked against the passed in ID
  
  // returns true if there are any valid targets, false otherwise
  bool HasDesiredReactionTargets(const Robot& robot, bool matchAnyPose = false) const;
  
  // adds ids to the set which this behavior thinks we should react to. Doesn't remove or clear
  void GetDesiredReactionTargets(const Robot& robot, std::set<s32>& targets, bool matchAnyPose = false) const;  

  // find the target in the desired targets which has the lowest cost of looking at. That is, the one that is
  // "easiest" (for some definition of easiest defined in the cpp file) to TurnTowards. It returns true if one
  // was found, false otherwise
  bool GetBestTarget(const Robot& robot, s32& bestTarget, bool matchAnyPose = false) const;

  // tells this class that the robot has reacted to a given id, assuming the reaction happened at the last
  // time the object was observed
  void RobotReactedToId(const Robot& robot, s32 id);
  
  // handle delocalized message
  virtual void AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot) override final;
  virtual void AlwaysHandlePoseBasedInternal(const EngineToGameEvent& event, const Robot& robot) {};

  
private:

  using super = IReactionaryBehavior;

  using ReactionDataMap = std::map<s32, ReactionData >;
  
  bool ShouldReactToTarget(const Robot& robot, const ReactionDataMap::value_type& reactionPair, bool matchAnyPose) const;

  bool ShouldReactToTarget_poseHelper(const Pose3d& thisPair,
                                      const Pose3d& otherPair) const;

  ReactionDataMap _reactionData;
 
}; // class IBehaviorPoseBasedAcknowledgement

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_IBehaviorPoseBasedAcknowledgement_H__
