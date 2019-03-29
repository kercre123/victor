/**
 * File: BehaviorCountingAnimation.cpp
 *
 * Author: Andrew Stout
 * Created: 2018-11-19
 *
 * Description: A hopefully-reusable behavior that coordinates a counting animation.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorCountingAnimation.h"

#include "engine/actions/animActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/sayTextAction.h"
#include "clad/types/animationTrigger.h"
#include "util/console/consoleInterface.h"

#define LOG_CHANNEL "BehaviorCountingAnimation"

namespace Anki {
namespace Vector {

namespace {
  CONSOLE_VAR(int, kSlowLoopBeginSize_loops, "BehaviorCountingAnimation", -1);
  CONSOLE_VAR(int, kSlowLoopEndSize_loops, "BehaviorCountingAnimation", -1);
}

namespace JsonKeys {
  static const char* SlowLoopBeginSizeKey = "SlowLoopBeginSize";
  static const char* SlowLoopEndSizeKey = "SlowLoopEndSize";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCountingAnimation::InstanceConfig::InstanceConfig():
  slowLoopBeginSize_loops(1),
  slowLoopEndSize_loops(1),
  getInOddCounts(1),
  getInEvenCounts(2)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCountingAnimation::DynamicVariables::DynamicVariables():
  target(0),
  even(false),
  slowLoopBeginNumLoops(0),
  fastLoopNumLoops(0),
  slowLoopEndNumLoops(0),
  setTargetCalled(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCountingAnimation::BehaviorCountingAnimation(const Json::Value& config)
 : ICozmoBehavior(config)
{
  // read config into _iConfig
  if(ANKI_VERIFY(config.isMember(JsonKeys::SlowLoopBeginSizeKey),
                 "BehaviorCountingAnimation.Constructor.MissingSlowLoopBeginSize", "")) {
    if(ANKI_VERIFY(config[JsonKeys::SlowLoopBeginSizeKey].isUInt(),
                   "BehaviorCountingAnimation.Constructor.TypeError", "SlowLoopBeginSize is not a uint")) {
      _iConfig.slowLoopBeginSize_loops = config[JsonKeys::SlowLoopBeginSizeKey].asUInt();
    }
  }
  if(ANKI_VERIFY(config.isMember(JsonKeys::SlowLoopEndSizeKey),
                 "BehaviorCountingAnimation.Constructor.MissingSlowLoopEndSize", "")) {
    if(ANKI_VERIFY(config[JsonKeys::SlowLoopEndSizeKey].isUInt(),
                   "BehaviorCountingAnimation.Constructor.TypeError", "SlowLoopEndSize is not a uint")) {
      _iConfig.slowLoopEndSize_loops = config[JsonKeys::SlowLoopEndSizeKey].asUInt();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCountingAnimation::~BehaviorCountingAnimation()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCountingAnimation::InitBehavior()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorCountingAnimation::WantsToBeActivatedBehavior() const
{
  // make sure SetTarget has been called
  if (!_dVars.setTargetCalled) {
    LOG_WARNING("BehaviorCountingAnimation.WantsToBeActivated.SetCountTargetNotCalled",
        "SetCountTarget must be called before activating.");
  }
  return _dVars.setTargetCalled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCountingAnimation::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenOffTreads = true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCountingAnimation::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    JsonKeys::SlowLoopBeginSizeKey,
    JsonKeys::SlowLoopEndSizeKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCountingAnimation::SetCountTarget(uint32_t target, const std::string & announcement)
{
  _dVars.target = target;
  _dVars.announcement = announcement;
  _dVars.setTargetCalled = true;
  LOG_INFO("BehaviorCountingAnimation.SetCountTarget.SetCountTarget",
      "target set to %d with announcement %s", _dVars.target, _dVars.announcement.c_str());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCountingAnimation::SetCountParameters()
{
  // make sure SetTarget has been called
  if (!_dVars.setTargetCalled) {
    LOG_WARNING("BehaviorCountingAnimation.SetCountParameters.SetCountTargetNotCalled",
                "SetCountTarget must be called before activating. Target defaults to 0. Probably a mistake.");
  }
  // check console vars
  if(kSlowLoopBeginSize_loops >= 0){
    _iConfig.slowLoopBeginSize_loops = kSlowLoopBeginSize_loops;
  }
  if(kSlowLoopEndSize_loops >= 0){
    _iConfig.slowLoopEndSize_loops = kSlowLoopEndSize_loops;
  }

  // we assume _dVars.target has already been set (if not, we'll count to zero).
  uint32_t remaining = _dVars.target; // we'll "consume" as we go
  // set these up for the minimum case: zero. (these are also the defaults, but just to be explicit...
  _dVars.even = false; // yes, I know 0 is actually even. But in this case we want to count as little as possible.
  _dVars.slowLoopBeginNumLoops = 0;
  _dVars.fastLoopNumLoops = 0;
  _dVars.slowLoopEndNumLoops = 0;
  if ( remaining == 0 ) {
    // for debugging:
    LOG_INFO("BehaviorCountingAnimation.SetCountParameters.settings",
                "target: %d\neven:%d\nslowLoopBeginLoops: %d\nfastLoopNumLoops: %d\nslowLoopEndNumLoops: %d",
                _dVars.target, _dVars.even, _dVars.slowLoopBeginNumLoops, _dVars.fastLoopNumLoops, _dVars.slowLoopEndNumLoops);
    return;
  }
  // even or odd - odd uses one, even uses two
  if ( remaining % 2 == 0 ) {
    _dVars.even = true;
    remaining -= _iConfig.getInEvenCounts;
  } else {
    _dVars.even = false;
    remaining -= _iConfig.getInOddCounts;
  }
  // assert that remaining is even (sanity check)
  DEV_ASSERT(remaining % 2 == 0, "BehaviorCountingAnimation.SetCountParameters.RemainingShouldBeEven");
  // beginning slow loop
  if ( remaining < _iConfig.slowLoopBeginSize_loops*2 ) {
    _dVars.slowLoopBeginNumLoops = remaining/2; // divide by 2 because the loop has two counts
    remaining = 0;
    // we've consumed the entire target; we're done.
    // for debugging:
    LOG_INFO("BehaviorCountingAnimation.SetCountParameters.settings",
                "target: %d\neven:%d\nslowLoopBeginLoops: %d\nfastLoopNumLoops: %d\nslowLoopEndNumLoops: %d",
                _dVars.target, _dVars.even, _dVars.slowLoopBeginNumLoops, _dVars.fastLoopNumLoops, _dVars.slowLoopEndNumLoops);
    return;
  }
  _dVars.slowLoopBeginNumLoops = _iConfig.slowLoopBeginSize_loops;
  remaining -= _iConfig.slowLoopBeginSize_loops*2; // multiply by 2 because the loop has two counts
  // assert that remaining is even (sanity check)
  DEV_ASSERT(remaining % 2 == 0, "BehaviorCountingAnimation.SetCountParameters.RemainingShouldBeEven");
  // ending slow loop
  if ( remaining < _iConfig.slowLoopEndSize_loops*2 ) {
    _dVars.slowLoopEndNumLoops = remaining/2; // divide by 2 because the loop has two counts
    remaining = 0;
    // we've consumed the entire target; we're done.
    // for debugging:
    LOG_INFO("BehaviorCountingAnimation.SetCountParameters.settings",
                "target: %d\neven:%d\nslowLoopBeginLoops: %d\nfastLoopNumLoops: %d\nslowLoopEndNumLoops: %d",
                _dVars.target, _dVars.even, _dVars.slowLoopBeginNumLoops, _dVars.fastLoopNumLoops, _dVars.slowLoopEndNumLoops);
    return;
  }
  _dVars.slowLoopEndNumLoops = _iConfig.slowLoopEndSize_loops;
  remaining -= _iConfig.slowLoopEndSize_loops*2; // multiply by 2 because the loop has two counts
  // assert that remaining is even (sanity check)
  DEV_ASSERT(remaining % 2 == 0, "BehaviorCountingAnimation.SetCountParameters.RemainingShouldBeEven");
  // remainder in the fast loop
  _dVars.fastLoopNumLoops = remaining/2; // divide by 2 because the loop has two counts
  remaining = 0;

  // for debugging:
  LOG_INFO("BehaviorCountingAnimation.SetCountParameters.settings",
      "target: %d\neven:%d\nslowLoopBeginLoops: %d\nfastLoopNumLoops: %d\nslowLoopEndNumLoops: %d",
      _dVars.target, _dVars.even, _dVars.slowLoopBeginNumLoops, _dVars.fastLoopNumLoops, _dVars.slowLoopEndNumLoops);

  return;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCountingAnimation::OnBehaviorActivated()
{

  SetCountParameters();

  CompoundActionSequential* animationSequence = new CompoundActionSequential();
  // get in
  // right now we're doing a get-in for zero, even though the smallest get-in counts to 1.
  // I'd like to fix that at some point, but it requires changes to the animations
  IAction* getIn = new TriggerLiftSafeAnimationAction( _dVars.even ? AnimationTrigger::CountingGetInEven
                                                                    : AnimationTrigger::CountingGetInOdd);
  animationSequence->AddAction(getIn);

  // slow loop begin
  if ( _dVars.slowLoopBeginNumLoops > 0 ) {
    IAction* slowLoopBegin = new TriggerLiftSafeAnimationAction(AnimationTrigger::CountingSlowLoop,
        _dVars.slowLoopBeginNumLoops);
    animationSequence->AddAction(slowLoopBegin);
  }

  // fast loop
  if ( _dVars.fastLoopNumLoops > 0 ) {
    //IAction* fastLoop = new ReselectingLoopAnimationAction(AnimationTrigger::CountingFastLoop,
    //    _dVars.fastLoopNumLoops);
    IAction* fastLoop = new TriggerLiftSafeAnimationAction(AnimationTrigger::CountingFastLoop,
                                                           _dVars.fastLoopNumLoops);
    animationSequence->AddAction(fastLoop);
  }

  // slow loop end
  if ( _dVars.slowLoopEndNumLoops > 0 ) {
    IAction* slowLoopEnd = new TriggerLiftSafeAnimationAction(AnimationTrigger::CountingSlowLoop,
        _dVars.slowLoopEndNumLoops);
    animationSequence->AddAction(slowLoopEnd);
  }

  // get out - we always do the get out
  IAction* getOut;
  if ( _dVars.announcement == "" ) {
    // empty announcement, no need to do the TTS thing
    getOut = new TriggerLiftSafeAnimationAction(AnimationTrigger::CountingGetOut);
  } else {
    auto* getOutSTA = new SayTextAction( _dVars.announcement );
    getOutSTA->SetAnimationTrigger( AnimationTrigger::CountingGetOut );
    getOut = getOutSTA;
    // note that doing it like this puts the potential for a slight delay (while the TTS audio is prepared)
    // before the get-out. The other option is to wrap the whole sequence in the SayTextAction, which would put
    // the delay at the beginning of the whole thing
    // Or we could manage all the TTS state ourselves (using TextToSpeechCoordinator) directly.
    // This seems like a fine approach for the time being.
  }
  animationSequence->AddAction(getOut);

  DelegateIfInControl( animationSequence );

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCountingAnimation::OnBehaviorDeactivated()
{
  // reset dynamic variables.
  // We do this here rather than on activation because we need to set the target before activation.
  _dVars = DynamicVariables();
}


}
}
