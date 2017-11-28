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

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "util/console/consoleInterface.h"

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

}

BehaviorVictorDemoNapping::BehaviorVictorDemoNapping(const Json::Value& config)
  : ICozmoBehavior(config)
{
}

bool BehaviorVictorDemoNapping::CanBeGentlyInterruptedNow(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return !_animIsPlaying;
}

Result BehaviorVictorDemoNapping::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  _animIsPlaying = false;
  
  TransitionToSleeping(behaviorExternalInterface);
  
  return Result::RESULT_OK;
}

void BehaviorVictorDemoNapping::TransitionToSleeping(BehaviorExternalInterface& behaviorExternalInterface)
{
  SetDebugStateName("sleeping");
  
  _numRemainingInBout = GetRNG().RandIntInRange(kSleepingBoutNumStirs_min, kSleepingBoutNumStirs_max);
    
  const float waitTime_s = GetRNG().RandDblInRange(kSleepingStirSpacing_min_s, kSleepingStirSpacing_max_s);
  DelegateIfInControl(new WaitAction(waitTime_s), &BehaviorVictorDemoNapping::TransitionToBoutOfStirring);
}

void BehaviorVictorDemoNapping::TransitionToBoutOfStirring(BehaviorExternalInterface& behaviorExternalInterface)
{
  SetDebugStateName("inBout");

  _animIsPlaying = false;

  if( _numRemainingInBout-- >= 0 ) {
    // start bout (wait first, then animate)    
    const float waitTime_s = GetRNG().RandDblInRange(kSleepingBoutSpacing_min_s, kSleepingBoutSpacing_max_s);
    DelegateIfInControl(new WaitAction(waitTime_s), &BehaviorVictorDemoNapping::TransitionToPlayStirAnim);
  }
  else {
    // back to sleep
    DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::GoToSleepOff),
                        &BehaviorVictorDemoNapping::TransitionToSleeping);
  }

}

void BehaviorVictorDemoNapping::TransitionToPlayStirAnim(BehaviorExternalInterface& behaviorExternalInterface)
{
  SetDebugStateName("stirring");
  _animIsPlaying = true;

  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::GoToSleepSleeping),
                      &BehaviorVictorDemoNapping::TransitionToBoutOfStirring);  
}


}
}
