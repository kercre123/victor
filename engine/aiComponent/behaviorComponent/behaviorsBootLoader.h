/**
 * File: behaviorsBootLoader.h
 *
 * Author: ross
 * Created: 2018 jun 2
 *
 * Description: Component that decides what behavior stack to initialize upon engine loading. This is
 *              based on factory state and dev options, but primarily is based on onboarding state.
 *              Since onboarding only occurs once in a robot's lifetime, this component "reboots"
 *              the behavior stack when onboarding ends, instead of having one giant stack containing
 *              both onboarding and regular behaviors.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_BehaviorsBootLoader_H__
#define __Engine_AiComponent_BehaviorComponent_BehaviorsBootLoader_H__

#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include <list>

namespace Anki {
  
namespace Util {
  class IConsoleFunction;
}
  
namespace Vector {

class BehaviorContainer;
class BehaviorSystemManager;
class IBehavior;
class IExternalInterface;
class IGatewayInterface;
class UnitTestKey;
enum class OnboardingStages : uint8_t;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BehaviorsBootLoader : public IDependencyManagedComponent<BCComponentID>
                          , public Anki::Util::noncopyable
{
public:
  explicit BehaviorsBootLoader(const Json::Value& config);
  
  // for unit tests to override
  BehaviorsBootLoader(IBehavior* overrideBehavior, const UnitTestKey& key);

  virtual void GetInitDependencies( BCCompIDSet& dependencies ) const override {
    dependencies.insert(BCComponentID::BehaviorContainer);
  }
  virtual void GetUpdateDependencies( BCCompIDSet& dependencies ) const override {
    dependencies.insert(BCComponentID::BehaviorSystemManager);
  }
  
  virtual void InitDependent( Robot* robot, const BCCompMap& dependentComps ) override;
  virtual void UpdateDependent(const BCCompMap& dependentComps) override;
  
  // Returns the behavior that should be at the base of the stack. Note that this may not be the
  // current behavior at the base of the stack if some other component has reset the stack recently,
  // or if there is a "pending" behavior that should be the next base behavior upon reset
  IBehavior* GetBootBehavior();

private:
  
  void InitOnboarding();
  
  void RestartOnboarding();
  
  void SetNewBehavior(BehaviorID behavior, bool requestStackReset = true);
  
  IExternalInterface* _externalInterface = nullptr;
  IGatewayInterface* _gatewayInterface = nullptr;
  const BehaviorContainer* _behaviorContainer = nullptr;
  
  IBehavior* _bootBehavior = nullptr;
  IBehavior* _behaviorToSwitchTo = nullptr;
  IBehavior* _overrideBehavior = nullptr;
  
  std::vector<Signal::SmartHandle> _eventHandles;
  
  bool _hasGrabbedBootBehavior = false;
  
  std::string _saveFolder;
  
  struct Behaviors {
    BehaviorID factoryBehavior; // when in factory
    BehaviorID onboardingBehavior; // onboarding
    BehaviorID normalBaseBehavior; // normal use behavior tree
    BehaviorID postOnboardingBehavior; // normal use behavior tree, but doesn't wake up
    BehaviorID devBaseBehavior; // for people who want it to turn on, eyes too, but not go anywhere
    BehaviorID prDemoBehavior; // for the pr demo
  };
  
  Behaviors _behaviors;
  
  OnboardingStages _stage;
  
  int _countUntilResetOnboarding = 0;
  std::list<Anki::Util::IConsoleFunction> _consoleFuncs;
};

}
}

#endif // __Engine_AiComponent_BehaviorComponent_BehaviorsBootLoader_H__
