/**
 * File: onboardingStageApp.h
 *
 * Author: ross
 * Created: 2018-06-07
 *
 * Description: Onboarding logic unit for app onboarding. This only runs if the app onboarding
 *              immediately follows robot onboarding. This stage just prevents the robot doing
 *              anything too crazy (unprompted fistbump) until a voice command is pending.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_Onboarding_OnboardingStageApp__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_Onboarding_OnboardingStageApp__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/iOnboardingStage.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"

namespace Anki {
namespace Cozmo {
  
class OnboardingStageApp : public IOnboardingStage
{
public:
  virtual ~OnboardingStageApp() = default;
  
  virtual void GetAllDelegates( std::set<BehaviorID>& delegates ) const override
  {
    delegates.insert( BEHAVIOR_ID(OnboardingLookAtUser) );
  }
  
  IBehavior* GetBehavior( BehaviorExternalInterface& bei ) override
  {
    return _selectedBehavior;
  }
  
  virtual void OnBegin( BehaviorExternalInterface& bei ) override
  {
    _selectedBehavior = GetBehaviorByID( bei, BEHAVIOR_ID(OnboardingLookAtUser) );
    
    // disable trigger word until continue
    DebugTransition("Waiting on continue to begin");
    SetTriggerWordEnabled(false);
  }
  
  virtual void OnContinue( BehaviorExternalInterface& bei ) override
  {
    DebugTransition("Waiting on voice command");
    // enable trigger word
    SetTriggerWordEnabled(true);
  }
  
  virtual void OnSkip( BehaviorExternalInterface& bei ) override
  {
    DebugTransition( "Skipped. Complete." );
    // all skips move to the next stage (which in this case is normal operation)
    _selectedBehavior = nullptr;
  }
  
  virtual void OnBehaviorDeactivated( BehaviorExternalInterface& bei ) override
  {
    DEV_ASSERT(false, "OnboardingStageApp.OnBehaviorDeactivated.UnexpectedDeactivation");
  }
  
  
  virtual void Update( BehaviorExternalInterface& bei ) override
  {
    const auto& uic = bei.GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();
    if( uic.IsAnyUserIntentPending() && !uic.IsUserIntentPending( USER_INTENT(unmatched_intent) ) ) {
      DebugTransition("User intent. Onboarding is done!");
      _selectedBehavior = nullptr;
    }
  }
  
private:
  void DebugTransition(const std::string& debugInfo)
  {
    PRINT_CH_INFO("Behaviors", "OnboardingStatus.App", "%s", debugInfo.c_str());
  }
  
  IBehavior* _selectedBehavior = nullptr;
};
  
} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_Onboarding_OnboardingStageApp__
