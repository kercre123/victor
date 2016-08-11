/**
 * File: behaviorReactToFrustration.h
 *
 * Author: Brad Neuman
 * Created: 2016-08-09
 *
 * Description: Behavior to react when the robot gets really frustrated (e.g. because he is failing actions)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToFrustration_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToFrustration_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "clad/types/animationTrigger.h"
#include <vector>

namespace Anki {
namespace Cozmo {

class BehaviorReactToFrustration : public IReactionaryBehavior
{
private:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToFrustration(Robot& robot, const Json::Value& config);

public:
  virtual bool IsRunnableInternalReactionary(const Robot& robot) const override { return true; }
  virtual bool ShouldResumeLastBehavior() const override { return false; }
  virtual bool ShouldComputationallySwitch(const Robot& robot) override;
  virtual bool ShouldInterruptOtherReactionaryBehavior() override { return false; }

protected:
  virtual Result InitInternalReactionary(Robot& robot) override;

private:

  struct FrustrationParams {
    explicit FrustrationParams(const Json::Value& config);
    
    // params
    float maxConfidentScore = 0.0f;
    AnimationTrigger anim = AnimationTrigger::Count;
    float randomDriveMinDist_mm = -1.0f;
    float randomDriveMaxDist_mm = -1.0f;
    float randomDriveMinAngle_deg = 0.0f;
    float randomDriveMaxAngle_deg = 0.0f;
    float cooldownTime_s = 0.0f;
    std::string finalEmotionEvent;
  };
    
  
  struct FrustrationReaction {
    explicit FrustrationReaction(const Json::Value& config) : params(config) {}

    const FrustrationParams params;
    float lastReactedTime_s = -1.0f;
  };

  using ReactionContainer = std::vector< FrustrationReaction >;

  void TransitionToReaction(Robot& robot, FrustrationReaction& reaction);
  void AnimationComplete(Robot& robot, FrustrationReaction& reaction);

  // returns end if no reaction, otherwise iter to the best one
  ReactionContainer::iterator GetReaction(const Robot& robot);

  // this is a map of frustration level -> reaction. We will react to the lowest Confident score here that is
  // >= the current Confident level, and not on cooldown (confidence is the opposite of frustration)
  ReactionContainer _possibleReactions;
  
};


} // namespace Cozmo
} // namespace Anki

#endif
