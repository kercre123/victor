/**
 * File: behaviorObservingWithoutTurn.h
 *
 * Author: Brad Neuman
 * Created: 2017-11-20
 *
 * Description: "idle" looking behavior to observe things without turning the body
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_Victor_BehaviorObservingWithoutTurn_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_Victor_BehaviorObservingWithoutTurn_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

enum class AnimationTrigger : int32_t;

class BehaviorObservingWithoutTurn : public ICozmoBehavior
{
public:

  virtual ~BehaviorObservingWithoutTurn();

protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorObservingWithoutTurn(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override{
    modifiers.wantsToBeActivatedWhenOnCharger = true;
    
    modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::Low});
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  virtual bool CanBeGentlyInterruptedNow() const override;
  virtual void OnBehaviorActivated() override;

  virtual bool WantsToBeActivatedBehavior() const override { return true; }

private:

  enum class State {
    LookingUp,
    LookingStraight
  };

  void TransitionToLookingUp();
  void TransitionToLookingStraight();
  
  void Loop();
  void SwitchState();

  // reset the timer to set when to play the next small head move (from the current time)
  void ResetSmallHeadMoveTimer();

  // when it's time for a small head move, this function will return which anim to use
  AnimationTrigger GetSmallHeadMoveAnim() const;

  void SetState_internal(State state, const std::string& stateName);

  float GetStateChangePeriod() const;
  
  State _state = State::LookingUp;

  float _lastStateChangeTime_s = 0.0f;
  float _lastSmallHeadMoveTime_s = 0.0f;

  // current length of time between small movements
  float _currSmallMovePeriod_s = 0.0f;

  // true while we're animating between states
  bool _isTransitioning = false;
  
  struct Params;
  std::unique_ptr<const Params> _params;

};

}
}


#endif
