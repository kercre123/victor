/**
 * File: behaviorReactToRobotShaken.h
 *
 * Author: Matt Michini
 * Created: 2017-01-11
 *
 * Description: Implementation of Dizzy behavior when robot is shaken
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToRobotShaken_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToRobotShaken_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorReactToRobotShaken : public ICozmoBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToRobotShaken(const Json::Value& config);
  
public:
  virtual bool WantsToBeActivatedBehavior() const override { return true;}
  
protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject  = true;
    modifiers.wantsToBeActivatedWhenOffTreads       = true;
    modifiers.behaviorAlwaysDelegates               = true;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;

  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Internal Data Structures and Functions ...

  using ReactionLevel = size_t;

  // Main behavior states:
  enum class EState : uint8_t
  {
    ShakeGetIn,
    Shaking,
    DoneShaking,
  };

  enum class EReactionAnimation : uint8_t
  {
    Loop,
    ShakeTransition,
    InHand,
    OnGround,
    Last = OnGround,
  };
  static constexpr uint8_t kNumReactionTypes = (int)EReactionAnimation::Last + 1;

  struct ShakeReaction
  {
    ShakeReaction( const Json::Value& config );

    float threshold;
    AnimationTrigger animations[kNumReactionTypes];
  };


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Helper Functions ...

  float GetShakeStopThreshold() const { return _iVars.shakeReactions[0].threshold; }

  // Reaction level helpers
  ReactionLevel GetReactionLevelFromMagnitude() const;
  void UpdateCurrentReactionLevel();

  // Animation related functions ...
  void LoadReactionAnimations( const Json::Value& config );
  AnimationTrigger GetReactionAnimation( EReactionAnimation type ) const;
  AnimationTrigger GetReactionAnimation( ReactionLevel level, EReactionAnimation type ) const;

  // Transition functions ...
  void PlayNextShakeReactionLoop();
  void TransitionToDoneShaking();


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Member Variables ...

  struct InstanceConfig
  {
    InstanceConfig();
    
    bool renderInEyeHue;
    AnimationTrigger getInAnimation;
    std::vector<ShakeReaction> shakeReactions;
  };

  struct DynamicVariables
  {
    EState state                = EState::ShakeGetIn;
    ReactionLevel currentLevel  = 0;

    float shakeMaxMagnitude     = 0.f;
    float shakeStartTime        = 0.f;
    float shakeEndTime          = 0.f;
  };

  DynamicVariables  _dVars;
  InstanceConfig    _iVars;
};

}
}

#endif
