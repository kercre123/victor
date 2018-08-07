/**
 * File: iOnboardingStage.h
 *
 * Author: ross
 * Created: 2018-06-02
 *
 * Description: A framework for the logic about how an onboarding stage should progress based on app
 *              messages, interruptions, any other messages, and dev buttons. This is not a behavior,
 *              but it can suggest what behavior should run. It uses explicit logic rather than latent
 *              transitions among behaviors to ensure a precise experience. IBehaviorOnboarding does
 *              all delegation and most message handling.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_Onboarding_iOnboardingStage__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_Onboarding_iOnboardingStage__
#pragma once

#include "util/helpers/noncopyable.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/userIntents.h"

namespace Anki {
namespace Cozmo {

class IOnboardingStage : private Anki::Util::noncopyable
{
public:
  virtual ~IOnboardingStage() = default;
  
  void Init() {
    _triggerEnabled = false;
    _whitelisted = USER_INTENT(INVALID);
  }
  
  // Return what behaviors this stage comprises
  virtual void GetAllDelegates( std::set<BehaviorID>& delegates ) const = 0;
  
  // Return what behavior should be active. If nullptr is returned, BehaviorOnboarding will move to
  // the next stage. This behavior MUST want to activate, or this stage ends.
  virtual IBehavior* GetBehavior( BehaviorExternalInterface& bei ) = 0;
  
  // Return what external_interface::OnboardingSteps the robot is expecting.
  virtual int GetExpectedStep() const = 0;
  
  // All the other calls below this will occur before GetBehavior() and GetExpectedStep()
  
  // called when this stage begins for the first time. This should reset all member vars
  virtual void OnBegin( BehaviorExternalInterface& bei ) {}
  
  // App (or dev tools) says "Continue"
  // Should return whether this stage accepts/handles the continue
  virtual bool OnContinue( BehaviorExternalInterface& bei, int stepNum ) = 0;
  
  // App (or dev tools) says "Skip"
  virtual void OnSkip( BehaviorExternalInterface& bei ) {}
  
  // The most recent behavior retrieved from GetBehavior finished
  virtual void OnBehaviorDeactivated( BehaviorExternalInterface& bei ) = 0;
  
  // called when this stage is interrupted, passing the interrupting behavior (e.g., TriggerWordDetected).
  // Return true if the stage should be considered complete despite the interruption. If true, it will
  // move to the next stage without resuming when the interruption is complete, and if false, it resume.
  virtual bool OnInterrupted( BehaviorExternalInterface& bei, BehaviorID interruptingBehavior ) { return false; }
  
  // called when this stage resumes from an interruption. This is called instead of OnBegin() if an
  // interrupting behavior had taken control after this stage already began (with OnBegin()).
  virtual void OnResume( BehaviorExternalInterface& bei, BehaviorID interruptingBehavior ) {}
  
  // Request and handle messages. Note that BehaviorOnboarding will handle all shared app and devtools
  // functionality like "Continue" and "Skip," so only subscribe to those tags specific to this stage.
  // Events will be received during the stage lifetime, starting after OnBegin() is called and continuing
  // until GetBehavior() returns nullptr, even if an interruption is running. OnMessage() will be called
  // before GetBehavior().
  virtual void GetAdditionalMessages( std::set<EngineToGameTag>& tags ) const {}
  virtual void GetAdditionalMessages( std::set<GameToEngineTag>& tags ) const {}
  virtual void GetAdditionalMessages( std::set<AppToEngineTag>& tags ) const {}
  virtual void OnMessage( BehaviorExternalInterface& bei, const EngineToGameEvent& event ) {}
  virtual void OnMessage( BehaviorExternalInterface& bei, const GameToEngineEvent& event ) {}
  virtual void OnMessage( BehaviorExternalInterface& bei, const AppToEngineEvent& event ) {}
  
  // Called each tick, after any of the above On* methods, but before GetBehavior()
  virtual void Update( BehaviorExternalInterface& bei ) {}
  
  // this gets checked by BehaviorOnboarding after Update. but before GetBehavior().
  bool GetWakeWordBehavior( UserIntentTag& onlyAllowedTag ) const
  {
    if( _triggerEnabled && (_whitelisted != USER_INTENT(INVALID)) ) {
      onlyAllowedTag = _whitelisted;
    }
    return _triggerEnabled;
  }
  
  
protected:
  // helper to get the behavior by ID
  IBehavior* GetBehaviorByID( BehaviorExternalInterface& bei, BehaviorID behaviorID )
  {
    auto behavior = bei.GetBehaviorContainer().FindBehaviorByID( behaviorID );
    ANKI_VERIFY( behavior != nullptr,
                 "IOnboardingStage.GetBehaviorByID.BehaviorNotFound",
                 "Behavior '%s' not found", BehaviorTypesWrapper::BehaviorIDToString(behaviorID) );
    return behavior.get();
  }
  
  // use these to set whether the trigger word is enabled
  void SetTriggerWordEnabled(bool enabled) { _triggerEnabled = enabled; }
  // if the trigger word is enabled, set whether just one intent is allowed or any intent
  void SetAllowedIntent( UserIntentTag intentTag ) { _whitelisted = intentTag; }
  void SetAllowAnyIntent() { _whitelisted = USER_INTENT(INVALID); }
private:
  bool _triggerEnabled = false;
  UserIntentTag _whitelisted = USER_INTENT(INVALID);
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_Onboarding_iOnboardingStage__
