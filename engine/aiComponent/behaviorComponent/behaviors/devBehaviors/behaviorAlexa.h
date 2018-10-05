/**
 * File: BehaviorAlexa.h
 *
 * Author: ross
 * Created: 2018-10-05
 *
 * Description: plays animations for various alexa UX states
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorAlexa__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorAlexa__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "clad/types/alexaUXState.h"

namespace Anki {
namespace Vector {

class BehaviorAlexa : public ICozmoBehavior
{
public: 
  virtual ~BehaviorAlexa();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorAlexa(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  
  virtual void OnBehaviorEnteredActivatableScope() override;
  
  virtual void AlwaysHandleInScope(const RobotToEngineEvent& event) override;

private:
  
  void TransitionToListeningLoop();
  void TransitionToListeningGetOut();
  
  enum class State : uint8_t {
    ListeningGetIn,
    ListeningLoop,
    ListeningGetOut_ToNothing,
  };

  struct InstanceConfig {
    InstanceConfig();
    // TODO: put configuration variables here
  };

  struct DynamicVariables {
    DynamicVariables();
    // TODO: put member variables here
    AlexaUXState uxState = AlexaUXState::Idle;
    State state = State::ListeningGetIn;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
  
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorAlexa__
