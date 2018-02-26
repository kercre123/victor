/**
 * File: behaviorObservingOnCharger.h
 *
 * Author: Brad Neuman
 * Created: 2017-11-20
 *
 * Description: "idle" looking behavior to observe things while Victor is sitting on the charger
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_Victor_BehaviorObservingOnCharger_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_Victor_BehaviorObservingOnCharger_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

enum class AnimationTrigger : int32_t;

class BehaviorObservingOnCharger : public ICozmoBehavior
{
public:

  virtual ~BehaviorObservingOnCharger();

protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorObservingOnCharger(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override{
    modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::Low});
    modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingMarkers, EVisionUpdateFrequency::Low });
  }
  virtual bool CanBeGentlyInterruptedNow() const override;
  virtual void OnBehaviorActivated() override;

  virtual bool WantsToBeActivatedBehavior() const override {
    return true;
  }

private:

  void TransitionToLookingUp();
  void TransitionToLookingStraight();
  
  void Loop();
  void SwitchState();

  // reset the timer to set when to play the next small head move (from the current time)
  void ResetSmallHeadMoveTimer();

  // when it's time for a small head move, this function will return which anim to use
  AnimationTrigger GetSmallHeadMoveAnim() const;
  
  enum class State {
    LookingUp,
    LookingStraight
  };

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
