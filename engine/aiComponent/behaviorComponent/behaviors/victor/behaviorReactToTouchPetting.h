/**
 * File: BehaviorReactToTouchPetting.h
 *
 * Author: Arjun Menon
 * Date:   09/12/2017
 *
 * Description: The robot's core reactions to repeated touches in the form of
 *              petting.
 *              This is embodied by increasing levels of bliss, with a getout
 *              for each level, and a final bliss cycle that loops forever until
 *              no more incoming touches
 *
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToTouchPetting_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToTouchPetting_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include <vector>

namespace Anki {
namespace Cozmo {
  
class IBEICondition;
class TriggerAnimationAction;
  
class BehaviorReactToTouchPetting : public ICozmoBehavior
{
public:
  
  virtual ~BehaviorReactToTouchPetting() { }
  virtual bool WantsToBeActivatedBehavior() const override;
  
protected:
  
  friend class BehaviorFactory;
  BehaviorReactToTouchPetting(const Json::Value& config);
  
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.behaviorAlwaysDelegates         = false;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  void InitBehavior() override;
  
  virtual void OnBehaviorActivated() override;

  virtual void OnBehaviorDeactivated() override;
  
  virtual void BehaviorUpdate() override;
  
  virtual void AlwaysHandleInScope(const EngineToGameEvent& event) override;
  
  void CancelAndPlayAnimation(AnimationTrigger anim);
  
  // helper method to play maximum bliss looping animations
  void PlayBlissLoopAnimation();
  
private:

  // - - - - - - - - - - - - - -
  // constants
  
  // ordered list of petting transition animations (all levels, incl. maxbliss)
  std::vector<AnimationTrigger> _animPettingResponse;
  
  // ordered list of petting getout animations (corresponds to above list)
  std::vector<AnimationTrigger> _animPettingGetout;
  
  AnimationTrigger _animPettingGetin;
  
  // duration of time to wait before checking
  // for state transition conditions for bliss
  // levels (i.e. num pets rcvd OR any active touch)
  float _timeTilTouchCheck;
  
  // the timeout for the max bliss state when
  // not receiving any touch input
  float _blissTimeout;
  
  // the timeout for any non-bliss state when
  // not receiving any touch input
  float _nonBlissTimeout;
  
  // mininum number of consecutive pets before
  // unlocking the opportunity to advance the
  // bliss level
  int _minNumPetsToAdvanceBliss;
  
  // - - - - - - - - - - - - -
  // mutable attributes
  
  enum PettingResponseState {
    PlayTransitionToLevel,
    PlayGetoutFromLevel,
    Done,
  };
  PettingResponseState _currResponseState;
  
  // future time after which we can end petting session (in the max bliss state)
  float _checkForTimeoutTimeBliss;
  
  // future time after which we can end petting session (in the non max-bliss state)
  float _checkForTimeoutTimeNonbliss;
  
  // future time after which we can check for transitions to higher bliss levels
  float _checkForTransitionTime;
  
  // tracking variable -- represents the current level of animation requested:
  // value 0 implies only the getin has been played
  // value 1-(N-1) maps to the petting transition animations, and their respective getout
  // value N maps to the maxbliss animation cycle, and its getout
  u32 _currBlissLevel;
  
  // tracking variable -- counting individual press/release pairs per bliss level
  u32 _numPressesAtCurrentBlissLevel;
  
  // counter for tracking ticks while pressed for audio purring
  // TODO: remove this in a future pass when audio system is setup
  u32 _numTicksPressed;
  
  // press state for the current tick
  bool _isPressed;
  
  // press state for the previous tick
  bool _isPressedPrevTick;
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToTouchPetting_H__
