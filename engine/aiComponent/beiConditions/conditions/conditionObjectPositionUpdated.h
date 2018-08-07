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

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionObjectPositionUpdated_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionObjectPositionUpdated_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/aiComponent/beiConditions/iBEIConditionEventHandler.h"

#include "coretech/common/engine/math/pose.h"
#include "coretech/common/engine/robotTimeStamp.h"
#include "anki/common/constantsAndMacros.h"
#include "coretech/common/shared/radians.h"

namespace Anki {
namespace Vector {

class BEIConditionMessageHelper;
  
namespace ExternalInterface {
struct RobotObservedObject;
}

class ConditionObjectPositionUpdated : public IBEICondition, private IBEIConditionEventHandler
{
public:
  ConditionObjectPositionUpdated(const Json::Value& config);

  virtual ~ConditionObjectPositionUpdated();

protected:
  virtual void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual void HandleEvent(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void GetRequiredVisionModes(std::set<VisionModeRequest>& requiredVisionModes) const override {
    requiredVisionModes.insert({ VisionMode::DetectingMarkers, EVisionUpdateFrequency::Low });
  }
private:
  // Default configuration parameters which can be overridden by JSON config
  struct StrategyParams {
    Radians     samePoseAngleThreshold_rad = DEG_TO_RAD_F32(45.f);
    TimeStamp_t coolDownDuration_ms        = 600000; // time since last seen (in same pose) before re-reaction
    float       samePoseDistThreshold_mm   = 80.f;
    float       samePoseDistThreshold_sparked_mm   = 30.f;
  } _params;
  
  // void LoadConfig(const Json::Value& config);
  
  struct ReactionData
  {
    Pose3d           lastPose;
    Pose3d           lastReactionPose;
    RobotTimeStamp_t lastSeenTime_ms;
    RobotTimeStamp_t lastReactionTime_ms;
    
    // helper function to fake a reaction (useful for cases where we want to ignore reactions)
    void FakeReaction();
  };
  
  void AddReactionData(s32 idToAdd, ReactionData&& data);
  bool RemoveReactionData(s32 idToRemove);
  
  // handles observing a new ID. Updates internal reaction data. If reactionEnabled is omitted, it checks
  // IsReactionEnabled(). If reactionEnabled is false, it will add the observation but ignore it for reactions
  // (by faking a reaction)
  void HandleNewObservation(s32 id, const Pose3d& pose, RobotTimeStamp_t timestamp, bool reactionEnabled = true);
  
  // For the next three functions, the bool `matchAnyPose` defaults to false. If true, then it checks poses
  // against any other pose, regardless of ID. If false, it only checks against it's own ID. Cooldown is still
  // checked against the passed in ID
  
  // returns true if there are any valid targets, false otherwise
  bool HasDesiredReactionTargets(BehaviorExternalInterface& behaviorExternalInterface, bool matchAnyPose = false) const;
  
  void HandleObjectObserved(BehaviorExternalInterface& behaviorExternalInterface, const ExternalInterface::RobotObservedObject& msg);
  
  
  using ReactionDataMap = std::map<s32, ReactionData >;
  
  bool ShouldReactToTarget(BehaviorExternalInterface& behaviorExternalInterface, const ReactionDataMap::value_type& reactionPair, bool matchAnyPose) const;
  
  bool ShouldReactToTarget_poseHelper(const Pose3d& thisPair,
                                      const Pose3d& otherPair) const;

  std::unique_ptr<BEIConditionMessageHelper> _messageHelper;
  
  mutable ReactionDataMap _reactionData;
};


} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionObjectPositionUpdated_H__
