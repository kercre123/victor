/**
 * File: BehaviorDevPettingTestSimple.h
 *
 * Author: Arjun Menon
 * Date:   09/12/2017
 *
 * Description: simple test behavior to respond to touch
 *              and petting input. Does nothing until a
 *              touch event comes in for it to react to
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorDevPettingTestSimple_H__
#define __Cozmo_Basestation_Behaviors_BehaviorDevPettingTestSimple_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include <vector>

namespace Anki {
namespace Cozmo {
  
class IBEICondition;
class TriggerAnimationAction;
  
class BehaviorDevPettingTestSimple : public ICozmoBehavior
{
public:
  
  virtual ~BehaviorDevPettingTestSimple() { }
  virtual bool WantsToBeActivatedBehavior() const override;
  
protected:
  
  friend class BehaviorFactory;
  BehaviorDevPettingTestSimple(const Json::Value& config);
  
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.behaviorAlwaysDelegates = false;
  }

  void InitBehavior() override;
  
  virtual void OnBehaviorActivated() override;

  virtual void OnBehaviorDeactivated() override;
  
  virtual void BehaviorUpdate() override;
  
  virtual void AlwaysHandleInScope(const EngineToGameEvent& event) override;

  void AudioStateMachine(int ticksVolumeIncFreq, int volumeLevelInc) const;
  
  void CancelAndPlayAction(TriggerAnimationAction* action, bool doCancelSelf = false);
  
private:

  // - - - - - - - - - - - - - -
  // constants
  std::vector<AnimationTrigger> _animTrigPetting;
  AnimationTrigger _animTrigPettingGetin;
  AnimationTrigger _animTrigPettingGetout;
  
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
  
  // internal setting to prevent certain tracks
  // in the animation reactions from playing
  u8 _tracksToLock;
  
  // - - - - - - - - - - - - -
  // mutable attributes
  
  float _checkForTimeoutTimeBliss;
  
  float _checkForTimeoutTimeNonbliss;
  
  // touch event bookmarked time (used to determine
  // when it is possible to start checking the
  // state transition conditions for the bliss state
  // machine)
  float _checkForTransitionTime;
  
  u32 _currBlissLevel;
  
  u32 _numPressesAtCurrentBlissLevel;
  
  u32 _numTicksPressed;
  
  bool _isPressed;
  
  bool _reachedBliss;
  
  bool _isPressedPrevTick;
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorDevPettingTestSimple_H__
