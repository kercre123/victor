/**
 * File: behaviorVictorDemoNapping.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-11-01
 *
 * Description: State to nap and stir occasionally in sleep
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/victor/behaviorVictorDemoNapping.h"

#include "anki/common/basestation/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "util/console/consoleInterface.h"

// includes needed for hack (see HoldFaceForTime and VIC-364)
#include "engine/robot.h"
#include "engine/actions/actionContainers.h"


namespace Anki {
namespace Cozmo {

namespace {

static const char* kConsoleGroup = "VictorDemoNapping";

CONSOLE_VAR_RANGED(f32, kSleepingStirSpacing_min_s, kConsoleGroup, 20.0f, 0.0f, 7200.0f);
CONSOLE_VAR_RANGED(f32, kSleepingStirSpacing_max_s, kConsoleGroup, 40.0f, 0.0f, 7200.0f);

CONSOLE_VAR_RANGED(f32, kSleepingBoutSpacing_min_s, kConsoleGroup, 1.5f, 0.0f, 30.0f);
CONSOLE_VAR_RANGED(f32, kSleepingBoutSpacing_max_s, kConsoleGroup, 8.0f, 0.0f, 7200.0f);

CONSOLE_VAR_RANGED(u32, kSleepingBoutNumStirs_min, kConsoleGroup, 2, 1, 10);
CONSOLE_VAR_RANGED(u32, kSleepingBoutNumStirs_max, kConsoleGroup, 5, 1, 10);

constexpr const char* kSleepingFaceLoopAnimClip = "anim_face_sleeping";

}

BehaviorVictorDemoNapping::BehaviorVictorDemoNapping(const Json::Value& config)
  : ICozmoBehavior(config)
{
}

bool BehaviorVictorDemoNapping::CanBeGentlyInterruptedNow(BehaviorExternalInterface& bei) const
{
  return !_animIsPlaying;
}

Result BehaviorVictorDemoNapping::OnBehaviorActivated(BehaviorExternalInterface& bei)
{
  _animIsPlaying = false;
  
  TransitionToSleeping(bei);
  
  return Result::RESULT_OK;
}

void BehaviorVictorDemoNapping::TransitionToSleeping(BehaviorExternalInterface& bei)
{
  SetDebugStateName("sleeping");
  
  _numRemainingInBout = GetRNG().RandIntInRange(kSleepingBoutNumStirs_min, kSleepingBoutNumStirs_max);
  
  const float waitTime_s = GetRNG().RandDblInRange(kSleepingStirSpacing_min_s, kSleepingStirSpacing_max_s);
  HoldFaceForTime(bei, waitTime_s, &BehaviorVictorDemoNapping::TransitionToBoutOfStirring);
}

void BehaviorVictorDemoNapping::TransitionToBoutOfStirring(BehaviorExternalInterface& bei)
{
  SetDebugStateName("inBout");

  _animIsPlaying = false;

  if( _numRemainingInBout-- >= 0 ) {
    // start bout (wait first, then animate)    
    const float waitTime_s = GetRNG().RandDblInRange(kSleepingBoutSpacing_min_s, kSleepingBoutSpacing_max_s);
    HoldFaceForTime(bei, waitTime_s, &BehaviorVictorDemoNapping::TransitionToPlayStirAnim);
  }
  else {
    // back to sleep
    TransitionToSleeping(bei);
  }

}

void BehaviorVictorDemoNapping::TransitionToPlayStirAnim(BehaviorExternalInterface& bei)
{
  SetDebugStateName("stirring");
  _animIsPlaying = true;

  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::GoToSleepSleeping),
                      &BehaviorVictorDemoNapping::TransitionToBoutOfStirring);  
}


void BehaviorVictorDemoNapping::HoldFaceForTime(
  BehaviorExternalInterface& bei,
  const float waitTime_s,
  void(BehaviorVictorDemoNapping::*callback)(BehaviorExternalInterface& bei))
{
  // This implementation is a huge hack which should go away as soon as VIC-364 is implemented so we can have
  // controls to directly turn off the procedural idles. In the meantime, we play an "engine-defined"
  // animation which keeps the face still, and has a side-effect of disabling procedural idles (since an
  // animation is playing). Then, after the given time, we cancel the animation. This hack was implemented
  // this way to give the impression that this is a single action (rather than implementing the cut-off in the
  // Update loop, which would require more code restructuring)

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _stopHoldingFaceAtTime_s = currTime_s + waitTime_s;

  LoopHoldFace(bei, callback);
}

void BehaviorVictorDemoNapping::LoopHoldFace(
  BehaviorExternalInterface& bei,
  void(BehaviorVictorDemoNapping::*callback)(BehaviorExternalInterface& bei))
{

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if( currTime_s > _stopHoldingFaceAtTime_s ) {
    (this->*callback)(bei);
  }
  else {
    // play one iteration of the animation, then check the time again
    DelegateIfInControl(new PlayAnimationAction(kSleepingFaceLoopAnimClip),
                        [this, callback](BehaviorExternalInterface& bei){
                          BehaviorVictorDemoNapping::LoopHoldFace(bei, callback);});
  }
}


}
}
