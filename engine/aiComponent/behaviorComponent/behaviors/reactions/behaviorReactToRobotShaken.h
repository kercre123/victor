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

#ifndef __Cozmo_Basestation_Behaviors_BeahviorReactToRobotShaken_H__
#define __Cozmo_Basestation_Behaviors_BeahviorReactToRobotShaken_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

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
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.behaviorAlwaysDelegates = false;
  }

  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;
  
private:

  // Main behavior states:
  enum class EState {
    Shaking,
    DoneShaking,
    WaitTilOnTreads,
    ActDizzy,
    Finished
  };
  
  EState _state = EState::Shaking;
  
  // The maximum filtered accelerometer magnitude encountered during the shaking event:
  float _maxShakingAccelMag = 0.f;

  float _shakingStartedTime_s = 0.f;
  float _shakenDuration_s = 0.f;

  // Possible Dizzy reactions:
  enum class EReaction {
    None,
    Soft,
    Medium,
    Hard,
    StillPickedUp
  };
  
  const char* EReactionToString(EReaction reaction) const;
  
  // The dizzy reaction that was played by this behavior:
  EReaction _reactionPlayed = EReaction::None;
  
};

}
}

#endif
