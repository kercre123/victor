/**
 * File: iOnboardingPhaseWithProgress.h
 *
 * Author: Sam Russell 
 * Created: 11/2018 
 *
 * Description: Interface class for behaviors to make their progress accessible to the OnboardingCoordinator. This
 *              allows the app to request progress reports from behaviors to key UX off of via RPC calls, which 
 *              will ideally allow us to execute onboarding over BLE if App<-WiFi->Robot is not working
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_IOnboardingPhaseWithProgress__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_IOnboardingPhaseWithProgress__

namespace Anki{
namespace Vector{

class IOnboardingPhaseWithProgress{
public:
  virtual int GetPhaseProgressInPercent() const = 0;
  // phases with internal state must be able to handle resuming in the event they are interrupted
  // externally since the App may be relying on their state
  virtual void ResumeUponNextActivation() = 0;
};

}//namespace Vector
}//namespace Anki

#endif //__Engine_AiComponent_BehaviorComponent_Behaviors_IOnboardingPhaseWithProgress__
