/**
 * File: onboardingStageWakeUpComeHere.h
 *
 * Author: ross
 * Created: 2018-06-02
 *
 * Description: The robot wakes up, drive off the charger, waits for a trigger word, waits for a second
 *              trigger word + "come here" voice command, plays a reaction
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_Onboarding_OnboardingStageWakeUpComeHere__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_Onboarding_OnboardingStageWakeUpComeHere__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/stages/iOnboardingStage.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntents.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {

class OnboardingStageWakeUpComeHere : public IOnboardingStage
{
public:
  virtual ~OnboardingStageWakeUpComeHere() = default;
  
  virtual void GetAllDelegates( std::set<BehaviorID>& delegates ) const override
  {
    delegates.insert( BEHAVIOR_ID(OnboardingLookAtPhone) );
    delegates.insert( BEHAVIOR_ID(OnboardingAsleep) );
    delegates.insert( BEHAVIOR_ID(OnboardingWakeUp) );
    delegates.insert( BEHAVIOR_ID(OnboardingSluggishDriveOffCharger) );
    delegates.insert( BEHAVIOR_ID(OnboardingWaitForTriggerWord) );
    delegates.insert( BEHAVIOR_ID(OnboardingWaitForComeHere) );
    delegates.insert( BEHAVIOR_ID(OnboardingComeHere) );
    delegates.insert( BEHAVIOR_ID(OnboardingComeHereResume) );
    delegates.insert( BEHAVIOR_ID(OnboardingComeHereGetOut) );
  }
  
  IBehavior* GetBehavior( BehaviorExternalInterface& bei ) override
  {
    return _currentBehavior;
  }
  
  virtual void OnBegin( BehaviorExternalInterface& bei ) override
  {
    SetTriggerWordEnabled(false);
    
    _behaviors.clear();
    _behaviors[Step::LookAtPhone]              = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingLookAtPhone) );
    _behaviors[Step::Asleep]                   = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingAsleep) );
    _behaviors[Step::WakingUp]                 = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingWakeUp) );
    _behaviors[Step::DriveOffCharger]          = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingSluggishDriveOffCharger) );
    _behaviors[Step::WaitingForTrigger]        = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingWaitForTriggerWord) );
    _behaviors[Step::WaitingForComeHere]       = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingWaitForComeHere) );
    _behaviors[Step::ComeHere]                 = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingComeHere) );
    _behaviors[Step::ComeHereGetOut]           = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingComeHereGetOut) );
    _behaviors[Step::ComeHereResume]           = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingComeHereResume) );
    
    _step = Step::LookAtPhone;
    _stepAfterResumeFromCharger = Step::LookAtPhone; // this is state is ignored since it could never happen outside initialization
    _currentBehavior = _behaviors[_step];
    DebugTransition("Waiting for OnboardingConnectionComplete to wake up");
  }
  
  virtual bool OnContinue( BehaviorExternalInterface& bei, OnboardingContinueEnum continueNum ) override
  {
    if( _step == Step::Asleep ) {
      TransitionToWakingUp();
    } else if( _step == Step::Complete ) {
      DEV_ASSERT(false, "OnboardingStageWakeUp.UnexpectedOnContinue");
    }
    return true;
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
    // stage is complete upon interruption if come here finished
    return (_step == Step::ComeHereResume) || (_step == Step::ComeHereGetOut);
  }
  
  virtual void OnResume( BehaviorExternalInterface& bei, BehaviorID interruptingBehavior ) override
  {
    const bool justGotTrigger = (_step == Step::WaitingForTrigger) && (interruptingBehavior == BEHAVIOR_ID(OnboardingFirstTriggerWord));
    auto* driveOffCharger = _behaviors[Step::DriveOffCharger];
    if( driveOffCharger->WantsToBeActivated() ) {
      DebugTransition("Driving off charger");
      if( justGotTrigger ) {
        _stepAfterResumeFromCharger = Step::WaitingForComeHere;
      } else {
        _stepAfterResumeFromCharger = _step;
      }
      _step = Step::DriveOffCharger;
      _currentBehavior = driveOffCharger;
    } else if( justGotTrigger ) {
      // successful trigger ==> resume from next step
      TransitionToWaitingForComeHere();
    } else if( (interruptingBehavior == BEHAVIOR_ID(ReactToCliff)) && (_step == Step::ComeHere) ) {
      // hit a cliff during come here, so something easier this time
      TransitionToComeHereResume();
    }
  }
  
  virtual void OnBehaviorDeactivated( BehaviorExternalInterface& bei ) override
  {
    // the previous behavior ended
    if( _step == Step::Complete ) {
      return;
    } else if( _step == Step::LookAtPhone ) {
      TransitionToAsleep();
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
    } else if( _step == Step::ComeHere ) {
      TransitionToComeHereGetOut();
    } else if( (_step == Step::ComeHereGetOut) || (_step == Step::ComeHereResume) ) {
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
    } else if( _step == Step::WaitingForComeHere ) {
      const auto& uic = bei.GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();
      if( uic.IsUserIntentPending( USER_INTENT(imperative_come) ) ) {
        // successful intent ==> next step (come here)
        TransitionToComeHere();
      }
    }
  }
  
private:
  
  void TransitionToAsleep()
  {
    DebugTransition("Asleep. Waiting for continue to wake up");
    _step = Step::Asleep;
    _currentBehavior = _behaviors[_step];
  }
  
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
  
  void TransitionToWaitingForComeHere()
  {
    DebugTransition("Waiting for come here voice command");
    _step = Step::WaitingForComeHere;
    _currentBehavior = _behaviors[_step];
    SetTriggerWordEnabled(true);
    SetAllowedIntent( USER_INTENT(imperative_come) );
  }
  
  void TransitionToComeHere()
  {
    DebugTransition("Running come here behavior");
    _step = Step::ComeHere;
    _currentBehavior = _behaviors[_step];
    // keep the trigger word the same, so that "hey vector come here" still works if the user tries to steer the robot
  }
  
  void TransitionToComeHereGetOut()
  {
    DebugTransition("Running come here getout");
    _step = Step::ComeHereGetOut;
    _currentBehavior = _behaviors[_step];
  }
  
  void TransitionToComeHereResume()
  {
    DebugTransition("Playing come here reaction");
    _step = Step::ComeHereResume;
    _currentBehavior = _behaviors[_step];
    SetTriggerWordEnabled(false);
  }
  
  void OnFinishedDrivingOffCharger()
  {
    switch( _stepAfterResumeFromCharger ) {
      case Step::LookAtPhone: // initialization value, meaning it was part of the regular wakeup sequence
      case Step::DriveOffCharger:
      case Step::WaitingForTrigger:
        TransitionToWaitingForTrigger();
        break;
      case Step::WakingUp:
        TransitionToWakingUp();
        break;
      case Step::WaitingForComeHere:
        TransitionToWaitingForComeHere();
        break;
      case Step::ComeHere:
        TransitionToComeHere();
        break;
      case Step::Asleep:
      case Step::Complete:
      case Step::ComeHereGetOut:
      case Step::ComeHereResume:
        DEV_ASSERT(false, "OnboardingStageWakeUpComeHere.UnexpectedDriveOffCharger");
        break;
    }
    _stepAfterResumeFromCharger = Step::LookAtPhone; // reset
  }
  
  void DebugTransition(const std::string& debugInfo)
  {
    PRINT_CH_INFO("Behaviors", "OnboardingStatus.WakeUpComeHere", "%s", debugInfo.c_str());
  }
  
  enum class Step : uint8_t {
    LookAtPhone=0,
    Asleep,
    WakingUp,
    DriveOffCharger,
    WaitingForTrigger,
    WaitingForComeHere,
    ComeHere,
    ComeHereGetOut, // reaction after coming here
    ComeHereResume, // if the robot hits a cliff during ComeHere, the resume behavior
    Complete, // waiting for cleaning
  };
  
  Step _step;
  IBehavior* _currentBehavior = nullptr;
  Step _stepAfterResumeFromCharger = Step::LookAtPhone;
  
  std::unordered_map<Step,IBehavior*> _behaviors;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_Onboarding_OnboardingStageWakeUp__
