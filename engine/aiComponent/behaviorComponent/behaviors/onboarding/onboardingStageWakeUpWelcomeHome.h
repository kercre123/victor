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
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntents.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {

class OnboardingStageWakeUpWelcomeHome : public IOnboardingStage
{
public:
  virtual ~OnboardingStageWakeUpWelcomeHome() = default;
  
  virtual void GetAllDelegates( std::set<BehaviorID>& delegates ) const override
  {
    delegates.insert( BEHAVIOR_ID(OnboardingAsleep) );
    delegates.insert( BEHAVIOR_ID(OnboardingWakeUp) );
    delegates.insert( BEHAVIOR_ID(OnboardingSluggishDriveOffCharger) );
    delegates.insert( BEHAVIOR_ID(OnboardingWaitForWelcomeHome) );
    delegates.insert( BEHAVIOR_ID(OnboardingReactToWelcomeHome) ); // this might get moved to the beginning of the next stage
  }
  
  IBehavior* GetBehavior( BehaviorExternalInterface& bei ) override
  {
    return _currentBehavior;
  }
  
  virtual void OnBegin( BehaviorExternalInterface& bei ) override
  {
    SetTriggerWordEnabled(false);
    
    _behaviors.clear();
    _behaviors[Step::Asleep]                 = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingAsleep) );
    _behaviors[Step::WakingUp]               = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingWakeUp) );
    _behaviors[Step::DriveOffCharger]        = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingSluggishDriveOffCharger) );
    _behaviors[Step::WaitingForTrigger]      = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingWaitForWelcomeHome) );
    _behaviors[Step::WaitingForWelcomeHome]  = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingWaitForWelcomeHome) );
    _behaviors[Step::WelcomeHomeReaction]    = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingReactToWelcomeHome) );
    
    _step = Step::Asleep;
    _stepAfterResumeFromCharger = Step::Asleep; // this is state is ignored since it could never happen outside initialization
    _currentBehavior = _behaviors[_step];
    DebugTransition("Waiting for continue to wake up");
  }
  
  virtual void OnContinue( BehaviorExternalInterface& bei ) override
  {
    if( _step == Step::Complete ) {
      return;
    } else if( _step == Step::Asleep ) {
      TransitionToWakingUp();
    } else {
      DEV_ASSERT(false, "OnboardingStageWakeUp.UnexpectedOnContinue");
    }
  }
  
  virtual void OnSkip( BehaviorExternalInterface& bei ) override
  {
    // skip always moves to the next stage (unable to speak trigger word, or unable to speak intent)
    DebugTransition("Skipped. Stage complete");
    _step = Step::Complete;
    _currentBehavior = nullptr;
  }
  
  virtual bool OnInterrupted( BehaviorExternalInterface& bei, BehaviorID interruptingBehavior ) override
  {
    // stage is complete upon interruption if welcome home finished
    return (_step == Step::WelcomeHomeReaction);
  }
  
  virtual void OnResume( BehaviorExternalInterface& bei, BehaviorID interruptingBehavior ) override
  {
    const bool justGotTrigger = (_step == Step::WaitingForTrigger) && (interruptingBehavior == BEHAVIOR_ID(TriggerWordDetected));
    auto* driveOffCharger = _behaviors[Step::DriveOffCharger];
    if( driveOffCharger->WantsToBeActivated() ) {
      DebugTransition("Driving off charger");
      if( justGotTrigger ) {
        _stepAfterResumeFromCharger = Step::WaitingForWelcomeHome;
      } else {
        _stepAfterResumeFromCharger = _step;
      }
      _step = Step::DriveOffCharger;
      _currentBehavior = driveOffCharger;
    } else if( justGotTrigger ) {
      // successful trigger ==> resume from next step
      TransitionToWaitingForWelcomeHome();
    }
  }
  
  virtual void OnBehaviorDeactivated( BehaviorExternalInterface& bei ) override
  {
    // the previous behavior ended
    if( _step == Step::Complete ) {
      return;
    } else if( _step == Step::WakingUp ) {
      // if on the charger, drive off
      _currentBehavior = _behaviors[Step::DriveOffCharger];
      if( _currentBehavior->WantsToBeActivated() ) {
        DebugTransition("Driving off charger");
        _step = Step::DriveOffCharger;
      } else {
        TransitionToWaitingForTrigger();
      }
    } else if( _step == Step::DriveOffCharger ) {
      OnFinishedDrivingOffCharger();
    } else if( _step == Step::WelcomeHomeReaction ) {
      // done
      DebugTransition("Stage complete");
      _step = Step::Complete;
      _currentBehavior = nullptr;
    } else {
      DEV_ASSERT(false, "OnboardingStageWakeUp.UnexpectedDeactivation");
    }
  }
  
  virtual void Update( BehaviorExternalInterface& bei ) override
  {
    if( _step == Step::DriveOffCharger ) {
      auto castPtr = dynamic_cast<ICozmoBehavior*>(_behaviors[Step::DriveOffCharger]);
      if( (castPtr != nullptr) && !castPtr->IsActivated() && !castPtr->WantsToBeActivated() ) {
        // Driving off the charger got canceled
        OnFinishedDrivingOffCharger();
      }
    } else if( _step == Step::Complete ) {
      return;
    } else if( _step == Step::WaitingForWelcomeHome ) {
      const auto& uic = bei.GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();
      if( uic.IsUserIntentPending( USER_INTENT(greeting_hello) ) ) {
        // successful intent ==> next step
        TransitionToWelcomeHomeReaction();
      }
    }
  }
  
private:
  
  void TransitionToWakingUp()
  {
    DebugTransition("WakingUp");
    _step = Step::WakingUp;
    _currentBehavior = _behaviors[_step];
  }
  
  void TransitionToWaitingForTrigger()
  {
    DebugTransition("Waiting for trigger word (no commands accepted)");
    _step = Step::WaitingForTrigger;
    _currentBehavior = _behaviors[_step];
    SetTriggerWordEnabled(true);
    // this blacklists everything. we want this one to fail.
    SetAllowedIntent( USER_INTENT(unmatched_intent) );
  }
  
  void TransitionToWaitingForWelcomeHome()
  {
    DebugTransition("Waiting for welcome home voice command");
    _step = Step::WaitingForWelcomeHome;
    _currentBehavior = _behaviors[_step];
    SetTriggerWordEnabled(true);
    SetAllowedIntent( USER_INTENT(greeting_hello) );
  }
  
  void TransitionToWelcomeHomeReaction()
  {
    DebugTransition("Playing welcome home reaction");
    _step = Step::WelcomeHomeReaction;
    _currentBehavior = _behaviors[_step];
    SetTriggerWordEnabled(false);
  }
  
  void OnFinishedDrivingOffCharger()
  {
    switch( _stepAfterResumeFromCharger ) {
      case Step::Asleep: // initialization value, meaning it was part of the regular wakeup sequence
      case Step::DriveOffCharger:
      case Step::WaitingForTrigger:
        TransitionToWaitingForTrigger();
        break;
      case Step::WakingUp:
        TransitionToWakingUp();
        break;
      case Step::WaitingForWelcomeHome:
        TransitionToWaitingForWelcomeHome();
        break;
      case Step::Complete:
      case Step::WelcomeHomeReaction:
        DEV_ASSERT(false, "OnboardingStageWakeUp.UnexpectedDriveOffCharger");
        break;
    }
    _stepAfterResumeFromCharger = Step::Asleep; // reset
  }
  
  void DebugTransition(const std::string& debugInfo)
  {
    PRINT_CH_INFO("Behaviors", "OnboardingStatus.WakeUpWelcomeHome", "%s", debugInfo.c_str());
  }
  
  enum class Step : uint8_t {
    Asleep=0,
    WakingUp,
    DriveOffCharger,
    WaitingForTrigger,
    WaitingForWelcomeHome,
    WelcomeHomeReaction,
    Complete,
  };
  
  Step _step;
  IBehavior* _currentBehavior = nullptr;
  Step _stepAfterResumeFromCharger = Step::Asleep;
  
  std::unordered_map<Step,IBehavior*> _behaviors;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_Onboarding_OnboardingStageWakeUp__
