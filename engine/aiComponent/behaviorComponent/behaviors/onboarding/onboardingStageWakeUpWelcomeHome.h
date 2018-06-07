/**
 * File: onboardingStageWakeUpWelcomeHome.h
 *
 * Author: ross
 * Created: 2018-06-02
 *
 * Description: The robot wakes up, drive off the charger, waits for a trigger word, waits for a second
 *              trigger word + "welcome home" voice command, plays a reaction
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_Onboarding_OnboardingStageWakeUpWelcomeHome__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_Onboarding_OnboardingStageWakeUpWelcomeHome__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/iOnboardingStage.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {

// todo: for now, this is just WakeUp so it leaves the charger

class OnboardingStageWakeUpWelcomeHome : public IOnboardingStage
{
public:
  virtual ~OnboardingStageWakeUpWelcomeHome() = default;
  
  virtual void GetAllDelegates( std::set<BehaviorID>& delegates ) const override
  {
    delegates.insert( BEHAVIOR_ID(OnboardingAsleep) );
    delegates.insert( BEHAVIOR_ID(OnboardingWakeUp) );
    delegates.insert( BEHAVIOR_ID(DriveOffCharger) );
  }
  
  IBehavior* GetBehavior( BehaviorExternalInterface& bei ) override
  {
    return _currentBehavior;
  }
  
  virtual void OnBegin( BehaviorExternalInterface& bei ) override
  {
    SetTriggerWordEnabled(false);
    
    _currentBehavior = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingAsleep) );
    _step = Step::Asleep;
  }
  
  virtual void OnContinue( BehaviorExternalInterface& bei ) override
  {
    if( _step == Step::Complete ) {
      return;
    } else if( _step == Step::Asleep ) {
      _currentBehavior = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingWakeUp) );
      _step = Step::WakingUp;
    } else {
      DEV_ASSERT(false, "OnboardingStageWakeUp.UnexpectedOnContinue");
    }
  }
  
  virtual void OnSkip( BehaviorExternalInterface& bei ) override
  {
    _step = Step::Complete;
    _currentBehavior = nullptr;
  }
  
  virtual void OnBehaviorDeactivated( BehaviorExternalInterface& bei ) override
  {
    // the previous behavior ended
    if( _step == Step::Complete ) {
      return;
    } else if( _step == Step::WakingUp ) {
      // if on the charger, drive off
      // todo: special drive off charger behavior
      _currentBehavior = GetBehaviorByID( bei, BEHAVIOR_ID(DriveOffCharger) );
      if( _currentBehavior->WantsToBeActivated() ) {
        _step = Step::DriveOffCharger;
      } else {
        // todo: the welcome home portion of this stage
        _step = Step::Complete;
        _currentBehavior = nullptr;
      }
    } else if( _step == Step::DriveOffCharger ) {
      _step = Step::Complete;
      _currentBehavior = nullptr;
    } else {
      DEV_ASSERT(false, "OnboardingStageWakeUp.UnexpectedDeactivation");
    }
  }
  
private:
  enum class Step : uint8_t {
    Asleep=0,
    WakingUp,
    DriveOffCharger,
    Complete,
  };
  
  Step _step;
  IBehavior* _currentBehavior = nullptr;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_Onboarding_OnboardingStageWakeUp__
