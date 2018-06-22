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
#include "engine/components/backpackLights/backpackLightComponent.h"
#include "engine/components/visionScheduleMediator/visionScheduleMediator_fwd.h"

#include <set>

namespace Anki {
  
namespace Util {
class IConsoleFunction;
}
  
namespace Cozmo {

class UnitTestKey;
  
class InternalStatesBehavior : public ICozmoBehavior
{
protected:  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  InternalStatesBehavior(const Json::Value& config);
  
  // subclasses of InternalStatesBehavior can pass handles to custom BEI conditions that can be used in
  // various BEIConditions in this class
  InternalStatesBehavior(const Json::Value& config, const CustomBEIConditionHandleList& customConditionHandles);

public:

  virtual ~InternalStatesBehavior();
  
protected:
  using StateID = size_t;
  static const StateID InvalidStateID = 0;
  
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void InitBehavior() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override { }
  virtual bool WantsToBeActivatedBehavior() const override { return true; }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  virtual void OverrideResumeState( StateID& resumeState ) const {}
  
  // helpers
  
  // asserts if not found
  StateID GetStateID(const std::string& stateName) const;
  // get current state
  StateID GetCurrentStateID() const { return _currState; }
  // the the time the state started (in seconds)
  float GetLastTimeStarted(StateID state) const;
  // the the time the state started (in seconds)
  float GetLastTimeEnded(StateID state) const;
  // returns true if state was exited at least timeout seconds ago. if valueIfNeverRun, this will
  // return true if state was never exited. if !trueIfNeverRun, then the cooldown timer counts up
  // from the engine start time
  bool StateExitCooldownExpired(StateID state, float timeout, bool valueIfNeverRun = true) const;
  
  // Set up a console var under uniqueVarName that transitions to each state type by name
  void AddConsoleVarTransitions(const char* uniqueVarName, const char* category );
  
  
private:
  
  class State;

  enum class TransitionType {
    // transitions that can happen when the currently delegated-to behavior thinks it's a decent time for a
    // gentle interruption (or if there is no currently delegated behavior)
    NonInterrupting,

    // transitions that can happen while the state is active (and in the middle of doing something)
    Interrupting,

    // exit transitions only run if the currently-delegated-to behavior stop itself. Note that these are
    // checked _after_ all of the other transitions
    Exit
  };

  static TransitionType TransitionTypeFromString(const std::string& str);

  // returns the newly created ID
  StateID AddStateName(const std::string& stateName);

  StateID ParseStateFromJson(const Json::Value& config, const std::string& key);
  IBEIConditionPtr ParseTransitionStrategy(const Json::Value& config);
  
  void AddState( State&& state );
  
  void TransitionToState(StateID targetState);

  // return the "adjusted" time of the current state (time it's been active - time it's been paused because
  // this behavior was interrupted)
  float GetCurrentStateActiveTime_s() const;

  CustomBEIConditionHandleList CreateCustomTimerConditions(const Json::Value& config);

  using NameToIdMapType = std::map< std::string, StateID >;
  NameToIdMapType _stateNameToID;

  using StateMap = std::map< StateID, State >;
  std::unique_ptr< StateMap > _states;

  StateID _currState = InvalidStateID;

  StateID _defaultState = InvalidStateID;

  bool _isRunningGetIn = false;
  
  size_t _lastTransitionTick = 0;
  
  std::vector<std::pair<StateID,StateID>> _resumeReplacements;

  BackpackLightAnimation::BackpackAnimation _currDebugLights;
  bool _debugLightsDirty = false;
  bool _useDebugLights = false;
  float _lastHearbeatLightTime = -1.0f;
  
  std::unique_ptr<Anki::Util::IConsoleFunction> _consoleFunc;
  StateID _consoleFuncState = InvalidStateID;
  
  bool _firstRun = true;
  
public:
  // for unit tests: grab a list of all conditions for each state name
  std::vector<std::pair<std::string, std::vector<IBEIConditionPtr>>> TESTONLY_GetAllTransitions( UnitTestKey key ) const;
  // check if the current state is running
  bool TESTONLY_IsStateRunning( UnitTestKey key, const std::string& name ) const;
};

}
}

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_InternalStateBehavior_H__
