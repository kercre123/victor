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

  // Main behavior states:
  enum class EState : uint8_t
  {
    ShakeGetIn,
    Shaking,
    DoneShaking,
  };

  enum class EReactionLevel : uint8_t
  {
    Soft,
    Medium,
    Hard,
  };

  enum class EReactionAnimation : uint8_t
  {
    Loop,
    ShakeTransition,
    InHand,
    OnGround,
  };

  // we can use either magnitude or duration to determine what shake level to use
  // adding both so we can easily go between the two during iteration
  EReactionLevel GetReactionLevelFromMagnitude() const;
//  EReactionLevel GetReactionLevelFromDuration() const;

  AnimationTrigger GetReactionAnimation( EReactionAnimation type ) const;
  AnimationTrigger GetReactionAnimation( EReactionLevel level, EReactionAnimation type ) const;

  void PlayNextShakeReactionLoop();
  void TransitionToDoneShaking();
  
  const char* EReactionLevelToString( EReactionLevel reaction ) const;


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Member Variables ...

  struct InstanceConfig
  {
    InstanceConfig();
    
    float shakeThresholdStop;
    float shakeThresholdMedium;
    float shakeThresholdHard;
  };

  struct DynamicVariables
  {
    EState state = EState::ShakeGetIn;

    float shakeMaxMagnitude   = 0.f;
    float shakeStartTime      = 0.f;
    float shakeEndTime        = 0.f;
  };

  DynamicVariables  _dVars;
  InstanceConfig    _iVars;
};

}
}

#endif
