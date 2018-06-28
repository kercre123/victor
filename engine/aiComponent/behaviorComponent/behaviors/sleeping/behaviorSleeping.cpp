/**
 * File: behaviorSleeping.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-11-01
 *
 * Description: State to nap and stir occasionally in sleep
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/sleeping/behaviorSleeping.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "util/console/consoleInterface.h"

// includes needed for hack (see HoldFaceForTime and VIC-364)
#include "engine/robot.h"
#include "engine/actions/actionContainers.h"


namespace Anki {
namespace Cozmo {

namespace {

#define CONSOLE_GROUP "Sleeping"

CONSOLE_VAR_RANGED(f32, kSleepingStirSpacing_min_s, CONSOLE_GROUP, 20.0f, 0.0f, 7200.0f);
CONSOLE_VAR_RANGED(f32, kSleepingStirSpacing_max_s, CONSOLE_GROUP, 40.0f, 0.0f, 7200.0f);

CONSOLE_VAR_RANGED(f32, kSleepingBoutSpacing_min_s, CONSOLE_GROUP, 1.5f, 0.0f, 30.0f);
CONSOLE_VAR_RANGED(f32, kSleepingBoutSpacing_max_s, CONSOLE_GROUP, 8.0f, 0.0f, 7200.0f);

CONSOLE_VAR_RANGED(u32, kSleepingBoutNumStirs_min, CONSOLE_GROUP, 2, 1, 10);
CONSOLE_VAR_RANGED(u32, kSleepingBoutNumStirs_max, CONSOLE_GROUP, 5, 1, 10);

constexpr const char* kSleepingFaceLoopAnimClip = "anim_face_sleeping";

}

BehaviorSleeping::BehaviorSleeping(const Json::Value& config)
  : ICozmoBehavior(config)
{
}

bool BehaviorSleeping::CanBeGentlyInterruptedNow() const
{
  return !_animIsPlaying;
}

void BehaviorSleeping::OnBehaviorActivated()
{
  _animIsPlaying = false;

  SmartRequestPowerSaveMode();
  
  TransitionToSleeping();  
}
  
void BehaviorSleeping::OnBehaviorDeactivated()
{
  PlayEmergencyGetOut(AnimationTrigger::WakeupGetout);
}

void BehaviorSleeping::TransitionToSleeping()
{
  SetDebugStateName("sleeping");
  
  _numRemainingInBout = GetRNG().RandIntInRange(kSleepingBoutNumStirs_min, kSleepingBoutNumStirs_max);
  
  const float waitTime_s = GetRNG().RandDblInRange(kSleepingStirSpacing_min_s, kSleepingStirSpacing_max_s);
  HoldFaceForTime(waitTime_s, &BehaviorSleeping::TransitionToBoutOfStirring);
}

void BehaviorSleeping::TransitionToBoutOfStirring()
{
  SetDebugStateName("inBout");

  _animIsPlaying = false;

  if( _numRemainingInBout-- >= 0 ) {
    // start bout (wait first, then animate)    
    const float waitTime_s = GetRNG().RandDblInRange(kSleepingBoutSpacing_min_s, kSleepingBoutSpacing_max_s);
    HoldFaceForTime(waitTime_s, &BehaviorSleeping::TransitionToPlayStirAnim);
  }
  else {
    // back to sleep
    TransitionToSleeping();
  }

}

void BehaviorSleeping::TransitionToPlayStirAnim()
{
  SetDebugStateName("stirring");
  _animIsPlaying = true;

  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::GoToSleepSleeping),
                      &BehaviorSleeping::TransitionToBoutOfStirring);  
}


void BehaviorSleeping::HoldFaceForTime(
  const float waitTime_s,
  void(BehaviorSleeping::*callback)())
{
  // This implementation is a huge hack which should go away as soon as VIC-364 is implemented so we can have
  // controls to directly turn off the procedural idles. In the meantime, we play an "engine-defined"
  // animation which keeps the face still, and has a side-effect of disabling procedural idles (since an
  // animation is playing). Then, after the given time, we cancel the animation. This hack was implemented
  // this way to give the impression that this is a single action (rather than implementing the cut-off in the
  // Update loop, which would require more code restructuring)

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _stopHoldingFaceAtTime_s = currTime_s + waitTime_s;

  LoopHoldFace(callback);
}

void BehaviorSleeping::LoopHoldFace(
  void(BehaviorSleeping::*callback)())
{

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if( currTime_s > _stopHoldingFaceAtTime_s ) {
    (this->*callback)();
  }
  else {
    // play one iteration of the animation, then check the time again
    DelegateIfInControl(new PlayAnimationAction(kSleepingFaceLoopAnimClip),
                        [this, callback](){
                          BehaviorSleeping::LoopHoldFace(callback);});
  }
}


}
}
