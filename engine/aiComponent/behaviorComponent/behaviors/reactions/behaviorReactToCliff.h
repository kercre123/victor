/**
 * File: behaviorReactToCliff.h
 *
 * Author: Kevin
 * Created: 10/16/15
 *
 * Description: Behavior for immediately responding to a detected cliff. This behavior actually handles both
 *              the stop and cliff events
 *
 * Copyright: Anki, Inc. 2015
 *
 **/
#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToCliff_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToCliff_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include <vector>

namespace Anki {
namespace Cozmo {

class ICompoundAction;
  
class BehaviorReactToCliff : public ICozmoBehavior
{
private:
  using super = ICozmoBehavior;
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorReactToCliff(const Json::Value& config);
  
public:  
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  
protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
  }

  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual void HandleWhileInScopeButNotActivated(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void HandleWhileActivated(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual void BehaviorUpdate(BehaviorExternalInterface& behaviorExternalInterface) override;

private:
  using base = ICozmoBehavior;
  enum class State {
    PlayingStopReaction,
    PlayingCliffReaction,
    BackingUp
  };

  State _state = State::PlayingStopReaction;

  bool _gotCliff = false;
  uint8_t _detectedFlags = 0;
  
  void TransitionToPlayingStopReaction(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToPlayingCliffReaction(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToBackingUp(BehaviorExternalInterface& behaviorExternalInterface);
  void SendFinishedReactToCliffMessage(BehaviorExternalInterface& behaviorExternalInterface);
  
  // Based on which cliff sensor(s) was tripped, select an appropriate pre-animation action
  CompoundActionSequential* GetCliffPreReactAction(BehaviorExternalInterface& behaviorExternalInterface, uint8_t cliffDetectedFlags);

  u16 _cliffDetectThresholdAtStart = 0;
  bool _quitReaction = false;
  
  bool _shouldStopDueToCharger;
  
  
}; // class BehaviorReactToCliff
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToCliff_H__
