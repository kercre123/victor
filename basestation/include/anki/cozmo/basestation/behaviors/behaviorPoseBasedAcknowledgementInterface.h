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
    AnimationTrigger reactionAnimTrigger   = AnimationTrigger::InteractWithFacesInitial;
    Radians     maxTurnAngle_rad           = DEG_TO_RAD_F32(45.f);
    Radians     samePoseAngleThreshold_rad = DEG_TO_RAD_F32(45.f);
    Radians     panTolerance_rad           = DEG_TO_RAD_F32(5.f);
    Radians     tiltTolerance_rad          = DEG_TO_RAD_F32(5.f);
    TimeStamp_t coolDownDuration_ms        = 600000; // time since last seen (in same pose) before re-reaction
    float       samePoseDistThreshold_mm   = 80.f;
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

  // adds ids to the set which this behavior thinks we should react to. Doesn't remove or clear
  void GetDesiredReactionTargets(const Robot& robot, std::set<s32>& targets) const;

  // tells this class that the robot has reacted to a given id, assuming the reaction happened at the last
  // time the object was observed
  void RobotReactedToId(const Robot& robot, s32 id);
  
private:

  using super = IReactionaryBehavior;

  std::map<s32, ReactionData > _reactionData;
 
}; // class IBehaviorPoseBasedAcknowledgement

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_IBehaviorPoseBasedAcknowledgement_H__
