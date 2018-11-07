/**
 * File: behaviorAlexa.h
 *
 * Author: ross
 * Created: 2018-11-01
 *
 * Description: Plays animations that mirror Alexa UX state
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorAlexa__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorAlexa__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/alexaComponentTypes.h"

namespace Anki {
namespace Vector {

enum class AlexaUXState : uint8_t;

class BehaviorAlexa : public ICozmoBehavior
{
public: 
  virtual ~BehaviorAlexa() = default;

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorAlexa(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual void OnBehaviorEnteredActivatableScope() override;
  virtual void OnBehaviorLeftActivatableScope() override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:
  
  void LoadConfig( const Json::Value& data );
  
  AnimationTrigger GetTransitionAnimation( AlexaUXState fromState, AlexaUXState toState ) const;
  AnimationTrigger GetLoopAnimation( AlexaUXState state ) const;
  
  void TransitionTo_IdleToNonIdle( AlexaUXState newState );
  void TransitionTo_StateLoop( AlexaUXState newState );
  void TransitionTo_Transition( AlexaUXState oldState, AlexaUXState newState ); // i heard you like transitions
  
  enum class AnimState : uint8_t {
    Idle,          // not yet active
    IdleToNonIdle, // waiting for an animProcess get-in to complete
    StateLoop,     // in a loop for Listening/Thinking/Speaking
    Transition,    // transitioning among Listening/Thinking/Speaking/Idle
  };
  
  using AnimTransitionList = std::vector< std::pair<AlexaUXState,AnimationTrigger> >;
  
  struct AnimInfo {
    AnimationTrigger loop;
    AnimTransitionList transitions;
  };
  
  struct InstanceConfig {
    InstanceConfig();
    
    AlexaResponseMap uxResponses;
    std::unordered_map<AlexaUXState,AnimInfo> animInfoMap;
  };

  struct DynamicVariables {
    DynamicVariables();
    
    AnimState animState;
    AlexaUXState uxState;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorAlexa__
