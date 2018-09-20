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

#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/stages/iOnboardingStage.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "proto/external_interface/onboardingSteps.pb.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Vector {
  
namespace {
  CONSOLE_VAR_RANGED(float, kTimeBeforeAppEnd_s, "Onboarding", 30.0f, 0.0f, 120.0f);
}
  
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
    _startTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    
    DebugTransition("Waiting on voice command or timeout");
    SetTriggerWordEnabled(true);
    SetAllowAnyIntent();
  }
  
  virtual bool OnContinue( BehaviorExternalInterface& bei, int stepNum ) override
  {
    return false;
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
    const float currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if( uic.IsAnyUserIntentPending() && !uic.IsUserIntentPending( USER_INTENT(unmatched_intent) ) ) {
      DebugTransition("User intent. Onboarding is done!");
      _selectedBehavior = nullptr;
    } else if( currTime - _startTime_s >= kTimeBeforeAppEnd_s ) {
      // user may have gotten to the point in app onboarding where it says to do a voice command, so break out
      DebugTransition("Timeout. Onboarding is done!");
      _selectedBehavior = nullptr;
    }
  }
  
  virtual int GetExpectedStep() const override
  {
    return external_interface::STEP_APP_ONBOARDING;
  }
  
private:
  void DebugTransition(const std::string& debugInfo)
  {
    PRINT_CH_INFO("Behaviors", "OnboardingStatus.App", "%s", debugInfo.c_str());
  }
  
  IBehavior* _selectedBehavior = nullptr;
  float _startTime_s = 0.0f;
};
  
} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_Onboarding_OnboardingStageApp__
