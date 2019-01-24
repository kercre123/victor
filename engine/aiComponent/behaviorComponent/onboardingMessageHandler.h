/**
 * File: onboardingMessageHandler.h
 * 
 * Author: Sam Russell
 * Date:   12/8/2018
 * 
 * Description: Receives and responds to onboarding related App messages appropriately based on onboarding state
 * 
 * Copyright: Anki, Inc. 2018
 * 
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_OnboardingMessageHandler_H__
#define __Engine_AiComponent_BehaviorComponent_OnboardingMessageHandler_H__

#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"
#include "clad/robotInterface/messageEngineToRobot.h"


#include <list>

namespace Anki{

namespace Util{
  class IConsoleFunction;
}

namespace Vector{

class BehaviorsBootLoader;
class IGatewayInterface;

class OnboardingMessageHandler : public IDependencyManagedComponent<BCComponentID>
                              , public Anki::Util::noncopyable
{
public:
  OnboardingMessageHandler();

  //IDependencyManagedComponent
  virtual void GetInitDependencies(BCCompIDSet& dependencies) const override {};
  virtual void AdditionalInitAccessibleComponents(BCCompIDSet& components) const override;
  virtual void GetUpdateDependencies(BCCompIDSet& dependencies) const override {};
  virtual void InitDependent(Robot* robot, const BCCompMap& dependentComps) override;
  virtual void UpdateDependent(const BCCompMap& dependentComps) override;
  //IDependencyManagedComponent

  void SubscribeToAppTags(std::set<AppToEngineTag>&& tags,
                          std::function<void(const AppToEngineEvent&)> messageHandler);

  void ShowUrlFace(bool show);
  void RequestBleSessions();

private:
  // - - - - - Message Handling functions - - - - -
  // These messages are handled ONLY locally by the OnboardingMessageHandler
  void HandleOnboardingStateRequest(const AppToEngineEvent& event);
  void HandleOnboardingCompleteRequest(const AppToEngineEvent& event);
  // - - - - - Message Handling functions - - - - -

  std::list<Anki::Util::IConsoleFunction> _consoleFuncs;

  BehaviorsBootLoader* _behaviorsBootLoader = nullptr;
  IGatewayInterface* _gatewayInterface = nullptr;

  std::vector<Signal::SmartHandle> _eventHandles;

  Robot* _robot = nullptr;

};

} //namespace Vector
} //namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_OnboardingMessageHandler_H__
