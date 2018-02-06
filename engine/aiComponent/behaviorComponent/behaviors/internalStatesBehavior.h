/**
 * File: internalStatesBehavior.h
 *
 * Author: Brad Neuman
 * Created: 2017-10-16
 *
 * Description: Behavior that manages an internal state machine for behavior delegation
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_InternalStateBehavior_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_InternalStateBehavior_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/components/bodyLightComponent.h"

namespace Anki {
namespace Cozmo {

class InternalStatesBehavior : public ICozmoBehavior
{
protected:
  // add a named strategy as a transition type that may be referenced in json
  using StrategyFunc = std::function<bool(BehaviorExternalInterface&)>;
  using PreDefinedStrategiesMap = std::unordered_map<std::string, StrategyFunc>;
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  InternalStatesBehavior(const Json::Value& config);
  
  // subclasses of InternalStatesBehavior can pass a map of predefined strategies
  InternalStatesBehavior(const Json::Value& config, PreDefinedStrategiesMap&& predefinedStrategies);
  
public:

  virtual ~InternalStatesBehavior();
  
protected:

  
  
  
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void InitBehavior() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override { }
  virtual bool WantsToBeActivatedBehavior() const override { return true; }
  
  using StateID = size_t;
  static const StateID InvalidStateID = 0;
  
  // helpers
  
  // asserts if not found
  StateID GetStateID(const std::string& stateName) const;
  // get current state
  StateID GetCurrentStateID() const { return _currState; }
  // the the time the state started (in seconds)
  float GetLastTimeStarted(StateID state) const;
  // the the time the state started (in seconds)
  float GetLastTimeEnded(StateID state) const;
  // true if at least time timeout has elapsed since the state has exited, or it has never exited before
  bool StateExitCooldownExpired(StateID state, float timeout) const;
  
private:

  void AddPreDefinedStrategy(const std::string& name, StrategyFunc&& func);
  
  class State;

  enum class TransitionType {
    NonInterrupting,
    Interrupting,
    Exit
  };

  static TransitionType TransitionTypeFromString(const std::string& str);

  // returns the newly created ID
  StateID AddStateName(const std::string& stateName);

  StateID ParseStateFromJson(const Json::Value& config, const std::string& key);
  IBEIConditionPtr ParseTransitionStrategy(const Json::Value& config);
  
  void AddState( State&& state );
  
  void TransitionToState(StateID targetState);

  std::map< std::string, StateID > _stateNameToID;

  using StateMap = std::map< StateID, State >;
  std::unique_ptr< StateMap > _states;

  std::map< std::string, IBEIConditionPtr > _preDefinedStrategies;

  StateID _currState = InvalidStateID;

  StateID _defaultState = InvalidStateID;

  BackpackLights _currDebugLights;
  bool _debugLightsDirty = false;
  bool _useDebugLights = false;
  float _lastHearbeatLightTime = -1.0f;
};

}
}

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_InternalStateBehavior_H__
