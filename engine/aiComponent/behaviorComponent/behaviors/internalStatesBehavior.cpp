/**
 * File: internalStatesBehavior.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-10-16
 *
 * Description: Behavior that manages an internal state machine for behavior delegation
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/internalStatesBehavior.h"

#include "coretech/common/engine/colorRGBA.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntents.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "engine/aiComponent/beiConditions/conditions/conditionLambda.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/unitTestKey.h"
#include "util/console/consoleFunction.h"
#include "util/console/consoleInterface.h"

#include <cctype>

namespace Anki {
namespace Cozmo {

namespace {

constexpr const char* kStateConfgKey = "states";
constexpr const char* kStateNameConfgKey = "name";


static const BackpackLights kLightsOff = {
  .onColors               = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
  .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
  .onPeriod_ms            = {{0,0,0}},
  .offPeriod_ms           = {{0,0,0}},
  .transitionOnPeriod_ms  = {{0,0,0}},
  .transitionOffPeriod_ms = {{0,0,0}},
  .offset                 = {{0,0,0}}
};

/** backpack debug lights description
 * on based on json param (see constructor)
 *
 * light 0: behavior state (front)
 * light 1: green = on charger, red = cliff detected (purple = both) (middle light)
 * light 2: blinking while update is running (status light)
 */

// NOTE: current victor lights are in a wonky order
static constexpr const u32 kDebugStateLED = 0;
static constexpr const u32 kDebugRobotStatusLED = 2;
static constexpr const u32 kDebugHeartbeatLED = 1;

static constexpr const float kHeartbeatPeriod_s = 0.6f;

}


class InternalStatesBehavior::State
{
public:
  
  State(const std::string& stateName, const std::string& behaviorName);
  explicit State(const Json::Value& config);

  // initialize this state after construction to fill in the behavior pointer
  void Init(BehaviorExternalInterface& bei);
  
  void AddInterruptingTransition(StateID toState, IBEIConditionPtr condition );
  void AddNonInterruptingTransition(StateID toState, IBEIConditionPtr condition );
  void AddExitTransition(StateID toState, IBEIConditionPtr condition);

  void OnActivated(BehaviorExternalInterface& bei);
  void OnDeactivated(BehaviorExternalInterface& bei);
  // TODO:(bn) add asserts for these

  // fills in allTransitions with the transitions present in this state
  void GetAllTransitions( std::set<IBEIConditionPtr>& allTransitions );
    
  std::string _name;
  std::string _behaviorName;
  ICozmoBehaviorPtr _behavior;

  float _lastTimeStarted_s = -std::numeric_limits<float>::min();
  float _lastTimeEnded_s = -std::numeric_limits<float>::min();

  // Transitions are evaluated in order, and if the function returns true, we will transition to the given
  // state id.
  using Transitions = std::vector< std::pair< StateID, IBEIConditionPtr > >;

  // transitions that can happen while the state is active (and in the middle of doing something)
  Transitions _interruptingTransitions;

  // transitions that can happen when the currently delegated-to behavior thinks it's a decent time for a
  // gentle interruption (or if there is no currently delegated behavior)
  Transitions _nonInterruptingTransitions;

  // exit transitions only run if the currently-delegated-to behavior stop itself. Note that these are
  // checked _after_ all of the other transitions
  Transitions _exitTransitions;

  // optional light debugging color
  ColorRGBA _debugColor = NamedColors::BLACK;
  
  UserIntentTag _clearIntent = USER_INTENT(INVALID);
};  

////////////////////////////////////////////////////////////////////////////////

InternalStatesBehavior::InternalStatesBehavior(const Json::Value& config, PreDefinedStrategiesMap&& predefinedTransitions)
  : ICozmoBehavior(config)
  , _states(new StateMap)
  , _currDebugLights(kLightsOff)
{
  
  _useDebugLights = config.get("use_debug_lights", true).asBool();

  // First, parse the state definitions to create all of the "state names" as a first pass
  for( const auto& stateConfig : config[kStateConfgKey] ) {
    AddStateName( JsonTools::ParseString(stateConfig, kStateNameConfgKey, "InternalStatesBehavior.StateConfig") );
  }

  for( auto& pair : predefinedTransitions ) {
    AddPreDefinedStrategy(pair.first, std::move(pair.second.first), pair.second.second );
  }
  
  // Parse the state config again to create the actual states
  for( const auto& stateConfig : config[kStateConfgKey] ) {
    State state(stateConfig);
    PRINT_CH_DEBUG("Behaviors", "HighLevelAI.LoadStateFromConfig",
                   "%s",
                   state._name.c_str());
    AddState(std::move(state));
  }

  DEV_ASSERT( _states->size() == _stateNameToID.size(),
              "HighLevelAI.StateConfig.InternalError.NotAllStatesDefined");

  ////////////////////////////////////////////////////////////////////////////////
  // Define transitions from json
  ////////////////////////////////////////////////////////////////////////////////

  // keep set of which states we've used so we can warn later if they aren't all used
  std::set< StateID > allFromStates;
  std::set< StateID > allToStates;
  
  for( const auto& transitionDefConfig : config["transitionDefinitions"] ) {
    std::vector<StateID> fromStates;
    if( transitionDefConfig["from"].isArray() ) {
      ANKI_VERIFY( transitionDefConfig["from"].size() > 0,
                   "InternalStatesBehavior.Ctor.EmptyArray",
                   "'from' array cannot be empty" );
      for( const auto& elem : transitionDefConfig["from"] ) {
        if( ANKI_VERIFY( elem.isString(),
                         "InternalStatesBehavior.Ctor.InvalidArray",
                         "'from' array does not contain strings" ) )
        {
          fromStates.push_back( GetStateID( elem.asString() ) );
        }
      }
    } else {
      fromStates.push_back( ParseStateFromJson(transitionDefConfig, "from") );
    }

    for( const StateID fromStateID : fromStates ) {
      if( allFromStates.find(fromStateID) == allFromStates.end() ) {
        allFromStates.insert(fromStateID);
      }

      State& fromState = _states->at(fromStateID);

      for( const auto& transitionConfig : transitionDefConfig["interruptingTransitions"] ) {
        StateID toState = ParseStateFromJson(transitionConfig, "to");
        IBEIConditionPtr strategy = ParseTransitionStrategy(transitionConfig);
        if( strategy ) {
          fromState.AddInterruptingTransition(toState, strategy);
          allToStates.insert(toState);
        }
      }

      for( const auto& transitionConfig : transitionDefConfig["nonInterruptingTransitions"] ) {
        StateID toState = ParseStateFromJson(transitionConfig, "to");
        IBEIConditionPtr strategy = ParseTransitionStrategy(transitionConfig);
        if( strategy ) {
          fromState.AddNonInterruptingTransition(toState, strategy);
          allToStates.insert(toState);
        }
      }

      for( const auto& transitionConfig : transitionDefConfig["exitTransitions"] ) {
        StateID toState = ParseStateFromJson(transitionConfig, "to");
        IBEIConditionPtr strategy = ParseTransitionStrategy(transitionConfig);
        if( strategy ) {
          fromState.AddExitTransition(toState, strategy);
          allToStates.insert(toState);
        }
      }
    }
  }

  if( allFromStates.size() != _states->size() ) {
    PRINT_NAMED_WARNING("HighLevelAI.TransitionDefinitions.DeadEndStates",
                        "Some states don't have any outgoing transition strategies! %zu from states, but %zu states",
                        allFromStates.size(),
                        _states->size());
  }

  if( allToStates.size() != _states->size() ) {
    PRINT_NAMED_WARNING("HighLevelAI.TransitionDefinitions.UnusedStates",
                        "Some states don't have any incoming transition strategies! %zu to states, but %zu states",
                        allToStates.size(),
                        _states->size());
  }

  PRINT_CH_INFO("Behaviors", "HighLevelAI.StatesCreated",
                "Created %zu states",
                _states->size());

  const std::string& initialStateStr = JsonTools::ParseString(config, "initialState", "InternalStatesBehavior.StateConfig");
  auto stateIt = _stateNameToID.find(initialStateStr);
  if( ANKI_VERIFY( stateIt != _stateNameToID.end(),
                   "InternalStatesBehavior.Config.InitialState.NoSuchState",
                   "State named '%s' does not exist",
                   initialStateStr.c_str()) ) {
    _currState = stateIt->second;
    _defaultState = stateIt->second;
  }

}
  
InternalStatesBehavior::InternalStatesBehavior(const Json::Value& config)
  : InternalStatesBehavior( config, {} )
{
}

InternalStatesBehavior::~InternalStatesBehavior()
{
}



void InternalStatesBehavior::InitBehavior()
{
  std::set< std::shared_ptr<IBEICondition> > allTransitions;
  
  // init all of the states
  for( auto& statePair : *_states ) {
    auto& state = statePair.second;
    state.Init(GetBEI());
    state.GetAllTransitions(allTransitions);
  }

  // initialize all transitions (from the set so they each only get initialized once)
  for( auto& strategy : allTransitions ) {
    strategy->Init(GetBEI());
  }

  PRINT_CH_INFO("Behaviors", "HighLevelAI.Init",
                "initialized %zu states",
                _states->size());
}

void InternalStatesBehavior::AddState( State&& state )
{
  if( ANKI_VERIFY( GetStateID(state._name) != InvalidStateID,
                   "InternalStatesBehavior.AddState.InvalidName",
                   "State has invalid name '%s'",
                   state._name.c_str()) &&
      ANKI_VERIFY( _states->find(GetStateID(state._name)) == _states->end(),
                   "InternalStatesBehavior.AddState.StateAlreadyExists", "") ) {
    _states->emplace(GetStateID(state._name), state);
  }
}

void InternalStatesBehavior::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  for( const auto& statePair : *_states ) {
    if( statePair.second._behavior != nullptr ) {
      delegates.insert(statePair.second._behavior.get());
    }
  }
}


void InternalStatesBehavior::OnBehaviorActivated()
{
  if( _useDebugLights ) {
    // force an update
    _debugLightsDirty = true;
  }

  // keep the state the same (either set from constructor or the last run)
  if( ANKI_VERIFY( _currState != InvalidStateID,
                   "InternalStatesBehavior.OnActivated.InvalidStateID",
                   "_currState set to invalid state id, falling back to initial state") ) {
    TransitionToState(_currState);
  }
  else {
    TransitionToState(_defaultState);
  }
}

void InternalStatesBehavior::OnBehaviorDeactivated()
{
  // hold the state in case we get re-activated, but still "transition" out of it (e.g. to disable vision
  // modes)
  // TODO:(bn) not this.
  const StateID endState = _currState;
  TransitionToState(InvalidStateID);
  _currState = endState;
}

void InternalStatesBehavior::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  if( _useDebugLights ) {
    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if( currTime_s - _lastHearbeatLightTime >= kHeartbeatPeriod_s ) {
      if( _currDebugLights.onColors[kDebugHeartbeatLED] == NamedColors::CYAN ) {
        _currDebugLights.onColors[  kDebugHeartbeatLED] =  NamedColors::BLACK;
        _currDebugLights.offColors[ kDebugHeartbeatLED] =  NamedColors::BLACK;
      }
      else {
        _currDebugLights.onColors[ kDebugHeartbeatLED] = NamedColors::CYAN;
        _currDebugLights.offColors[kDebugHeartbeatLED] = NamedColors::CYAN;
      }
      _lastHearbeatLightTime = currTime_s;
      _debugLightsDirty = true;
    }

    auto& robotInfo = GetBEI().GetRobotInfo();
    
    ColorRGBA robotStateColor = NamedColors::BLACK;
    if( robotInfo.IsOnChargerPlatform() ) {
      robotStateColor.SetG(1.0f);
    }
    if( robotInfo.GetCliffSensorComponent().IsCliffDetectedStatusBitOn() ) {
      robotStateColor.SetR(1.0f);
    }

    if( _currDebugLights.onColors[kDebugRobotStatusLED] != robotStateColor ) {
      _currDebugLights.onColors[  kDebugRobotStatusLED]  = robotStateColor;
      _currDebugLights.offColors[ kDebugRobotStatusLED]  = robotStateColor;
      
      _debugLightsDirty = true;
    }

    if( _debugLightsDirty ) {
      
      GetBEI().GetBodyLightComponent().SetBackpackLights(_currDebugLights);
      _debugLightsDirty = false;
    }
  }

  if( _currState == InvalidStateID ) {
    // initial actions must be running
    // TODO:(bn) make this a proper state?
    if(!IsControlDelegated()){
      CancelSelf();
    }
    return;
  }
  
  State& state = _states->at(_currState);
  
  if( ANKI_DEV_CHEATS ) {
    // check for console var transitions
    if( _consoleFuncState != InvalidStateID ) {
      const StateID stateID = _consoleFuncState;
      _consoleFuncState = InvalidStateID;
      TransitionToState(stateID);
      return;
    }
  }

  // first check the interrupting conditions
  for( const auto& transitionPair : state._interruptingTransitions ) {
    const auto stateID = transitionPair.first;
    const auto& iConditionPtr = transitionPair.second;

    if( iConditionPtr->AreConditionsMet(GetBEI()) ) {
      TransitionToState(stateID);
      return;
    }
  }

  // if we get here, then there must be no interrupting conditions that activated

  // it's ok to dispatch now if either we aren't dispatched to anything, or the behavior we are dispatched to
  // is ok with a "gentle" interruption
  bool okToDispatch = ! IsControlDelegated();

  if( !okToDispatch ) {
    if(GetBEI().HasDelegationComponent()){
      // TODO:(bn) rather than calling this directly, we can introduce a "stop yourself in X seconds" event
      // that ICozmoBehavior implements (and hears about from StateChangeComponent)
      auto& delegationComponent = GetBEI().GetDelegationComponent();
      const IBehavior* delegate = delegationComponent.GetBehaviorDelegatedTo(this);
      const ICozmoBehavior* delegateBehavior = dynamic_cast<const ICozmoBehavior*>(delegate);
      if( delegateBehavior ) {
        okToDispatch = delegateBehavior->CanBeGentlyInterruptedNow();
      }
    }
  }

  if( okToDispatch ) {
    for( const auto& transitionPair : state._nonInterruptingTransitions ) {
      const auto stateID = transitionPair.first;
      const auto& iConditionPtr = transitionPair.second;
      
      if( iConditionPtr->AreConditionsMet(GetBEI()) ) {
        TransitionToState(stateID);
        return;
      }
    }

    if( ! IsControlDelegated() ) {
      // Now there have been no interrupting or non-interrupting transitions, so check the exit conditions
      for( const auto& transitionPair : state._exitTransitions ) {
        const auto stateID = transitionPair.first;
        const auto& iConditionPtr = transitionPair.second;
      
        if( iConditionPtr->AreConditionsMet(GetBEI()) ) {
          TransitionToState(stateID);
          return;
        }
      }
    }

    // if we get here, then there is no state we want to switch to, so re-start the current one if it wants to
    // run and isn't already running
    
    // TODO:(bn) can behaviors be null?
    if( !IsControlDelegated() && state._behavior->WantsToBeActivated() ) {
      DelegateIfInControl(state._behavior.get() );
    }
    // else we'll just sit here doing nothing evaluating the conditions each tick
  }

  // This behavior never ends, it's always running
}

void InternalStatesBehavior::TransitionToState(const StateID targetState)
{

  // TODO:(bn) don't de- and re-activate behaviors if switching states doesn't change the behavior  

  if( _currState != InvalidStateID ) {
    _states->at(_currState).OnDeactivated(GetBEI());
    const bool allowCallback = false;
    CancelDelegates(allowCallback);
  }
  else {
    DEV_ASSERT( !IsControlDelegated() || targetState == InvalidStateID,
                "HighLevelAI.TransitionToState.WasInCountButHadDelegate" );
  }
          
  // TODO:(bn) channel for high level ai?
  PRINT_CH_INFO("Unfiltered", "HighLevelAI.TransitionToState",
                "Transition from state '%s' -> '%s'",
                _currState  != InvalidStateID ? _states->at(_currState )._name.c_str() : "<NONE>",
                targetState != InvalidStateID ? _states->at(targetState)._name.c_str() : "<NONE>");

  _currState = targetState;

  if( _currState != InvalidStateID ) {
    State& state = _states->at(_currState);

    state.OnActivated(GetBEI());

    if( _useDebugLights ) {
      if( _currDebugLights.onColors[kDebugStateLED] != state._debugColor ) {
        _debugLightsDirty = true;
        _currDebugLights.onColors[kDebugStateLED] = state._debugColor;
        _currDebugLights.offColors[kDebugStateLED] = state._debugColor;
      }
    }

    if( state._behavior->WantsToBeActivated() ) {
      DelegateIfInControl(state._behavior.get() );
    }
  }
}


InternalStatesBehavior::State::State(const std::string& stateName, const std::string& behaviorName)
  : _name(stateName)
  , _behaviorName(behaviorName)
{
}

InternalStatesBehavior::State::State(const Json::Value& config)
{
  _name = JsonTools::ParseString(config, kStateNameConfgKey, "InternalStatesBehavior.StateConfig");
  _behaviorName = JsonTools::ParseString(config, "behavior", "InternalStatesBehavior.StateConfig");

  const std::string& debugColorStr = config.get("debugColor", "BLACK").asString();
  _debugColor = NamedColors::GetByString(debugColorStr);
  
  const std::string& clearIntent = config.get("clearIntent", "").asString();
  if( !clearIntent.empty() ) {
    ANKI_VERIFY( UserIntentTagFromString( clearIntent, _clearIntent ),
                 "InternalStateBehavior.State.Ctor.InvalidIntent",
                 "Could not get user intent from '%s'",
                 clearIntent.c_str() );
  }
}

void InternalStatesBehavior::State::Init(BehaviorExternalInterface& bei)
{
  const auto& BC = bei.GetBehaviorContainer();
  _behavior = BC.FindBehaviorByID( BehaviorTypesWrapper::BehaviorIDFromString( _behaviorName ) );
  DEV_ASSERT_MSG(_behavior != nullptr, "HighLevelAI.State.NoBehavior",
                 "State '%s' cannot find behavior '%s'",
                 _name.c_str(),
                 _behaviorName.c_str());
}


void InternalStatesBehavior::State::GetAllTransitions( std::set<IBEIConditionPtr>& allTransitions )
{
  for( auto& transitionPair : _interruptingTransitions ) {
    allTransitions.insert(transitionPair.second);
  }
  for( auto& transitionPair : _nonInterruptingTransitions ) {
    allTransitions.insert(transitionPair.second);
  }
  for( auto& transitionPair : _exitTransitions ) {
    allTransitions.insert(transitionPair.second);
  }
}

void InternalStatesBehavior::State::AddInterruptingTransition(StateID toState,
                                                                   IBEIConditionPtr condition)
{
  // TODO:(bn) references / rvalue / avoid copies?
  _interruptingTransitions.emplace_back(toState, condition);
}

void InternalStatesBehavior::State::AddNonInterruptingTransition(StateID toState,
                                                                      IBEIConditionPtr condition )
{
  _nonInterruptingTransitions.emplace_back(toState, condition);
}

void InternalStatesBehavior::State::AddExitTransition(StateID toState, IBEIConditionPtr condition)
{
  _exitTransitions.emplace_back(toState, condition);
}

void InternalStatesBehavior::State::OnActivated(BehaviorExternalInterface& bei)
{
  if( _clearIntent != USER_INTENT(INVALID) ) {
    auto& uic = bei.GetAIComponent().GetBehaviorComponent().GetUserIntentComponent();
    if( uic.IsUserIntentPending( _clearIntent ) ) {
      uic.ClearUserIntent( _clearIntent );
    }
  }
  for( const auto& transitionPair : _interruptingTransitions ) {
    const auto& iConditionPtr = transitionPair.second;
    iConditionPtr->SetActive(bei, true);
  }
  for( const auto& transitionPair : _nonInterruptingTransitions ) {
    const auto& iConditionPtr = transitionPair.second;
    iConditionPtr->SetActive(bei, true);
  }
  for( const auto& transitionPair : _exitTransitions ) {
    const auto& iConditionPtr = transitionPair.second;
    iConditionPtr->SetActive(bei, true);
  }

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _lastTimeStarted_s = currTime_s;       
}

void InternalStatesBehavior::State::OnDeactivated(BehaviorExternalInterface& bei)
{
  for( const auto& transitionPair : _interruptingTransitions ) {
    const auto& iConditionPtr = transitionPair.second;
    iConditionPtr->SetActive(bei, false);
  }
  for( const auto& transitionPair : _nonInterruptingTransitions ) {
    const auto& iConditionPtr = transitionPair.second;
    iConditionPtr->SetActive(bei, false);
  }
  for( const auto& transitionPair : _exitTransitions ) {
    const auto& iConditionPtr = transitionPair.second;
    iConditionPtr->SetActive(bei, false);
  }

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _lastTimeEnded_s = currTime_s;
}

bool InternalStatesBehavior::StateExitCooldownExpired(StateID state, float timeout, bool valueIfNeverRun) const
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool neverRun = valueIfNeverRun && (_states->at(state)._lastTimeEnded_s < 0.0f);
  if( neverRun || (_states->at(state)._lastTimeEnded_s + timeout <= currTime_s) ) {
    return true;
  }
  else {
    return false;
  }
}


InternalStatesBehavior::StateID InternalStatesBehavior::AddStateName(const std::string& stateName)
{
  if( ANKI_VERIFY( _stateNameToID.find(stateName) == _stateNameToID.end(),
                   "InternalStatesBehavior.AddStateName.DuplicateName",
                   "Adding '%s' more than once",
                   stateName.c_str()) ) {
    const size_t newID = _stateNameToID.size() + 2; // +1 for the state itself, +1 to skip 0
    _stateNameToID[stateName] = newID;
    DEV_ASSERT(GetStateID(stateName) == newID, "InternalStatesBehavior.StateNameBug");
    return newID;
  }
  return InvalidStateID;
}
  
void InternalStatesBehavior::AddConsoleVarTransitions( const char* uniqueVarName, const char* category )
{
  if( ANKI_DEV_CHEATS ) {
    auto func = [this](ConsoleFunctionContextRef context) {
      const char* stateName = ConsoleArg_Get_String(context, "stateName");
      // case insensitive find for convenience
      auto tolower = [](const char c) { return std::tolower(c); };
      // lower cased request
      std::string requestLowerCase{stateName};
      std::transform(requestLowerCase.begin(), requestLowerCase.end(), requestLowerCase.begin(), tolower);
      // and make a lower-cased version of _stateNameToID
      NameToIdMapType lcStateNameToID;
      for( const auto& pair : _stateNameToID ) {
        std::string lcName(pair.first.size(),0);
        std::transform(pair.first.begin(), pair.first.end(), lcName.begin(), tolower);
        lcStateNameToID.emplace( lcName, pair.second );
      }
      // actually find the state
      const auto it = lcStateNameToID.find( requestLowerCase );
      if( it != lcStateNameToID.end() ) {
        const StateID stateID = it->second;
        if( stateID != InvalidStateID ) {
          // don't transition here, otherwise the tick timing doesn't
          // work out like if this were a normal transition. Instead,
          // something else is watching _consoleFuncState
          _consoleFuncState = stateID;
        }
      }
    };
    auto* cfunc = new Anki::Util::IConsoleFunction( uniqueVarName, std::move(func), category, "const char* stateName" );
    _consoleFunc.reset( cfunc );
  }
}

InternalStatesBehavior::StateID InternalStatesBehavior::GetStateID(const std::string& stateName) const
{
  auto it = _stateNameToID.find(stateName);
  if( ANKI_VERIFY( it != _stateNameToID.end(),
                   "InternalStatesBehavior.GetStateID.NoSuchState",
                   "State named '%s' does not exist",
                   stateName.c_str()) ) {
    return it->second;
  }
  return InvalidStateID;
}
  
float InternalStatesBehavior::GetLastTimeStarted(StateID state) const
{
  return _states->at(state)._lastTimeStarted_s;
}
  
float InternalStatesBehavior::GetLastTimeEnded(StateID state) const
{
  return _states->at(state)._lastTimeEnded_s;
}
  
void InternalStatesBehavior::AddPreDefinedStrategy(const std::string& name, 
                                                   StrategyFunc&& strategyFunc,
                                                   std::set<VisionModeRequest>& requiredVisionModes)
{
  ANKI_VERIFY( _preDefinedStrategies.find(name) == _preDefinedStrategies.end(),
               "InternalStatesBehavior.AddPreDefinedStrategy.Duplicate",
               "Behavior '%s' is adding duplicate strategy '%s'",
               GetDebugLabel().c_str(),
               name.c_str() );

  _preDefinedStrategies[name] = std::make_shared<ConditionLambda>(strategyFunc, requiredVisionModes);
  _preDefinedStrategies[name]->SetOwnerDebugLabel( GetDebugLabel() );
}

InternalStatesBehavior::StateID InternalStatesBehavior::ParseStateFromJson(const Json::Value& config,
                                                                                     const std::string& key)
{
  if( ANKI_VERIFY( config[key].isString(),
                   "HighLevelAI.ParseStateFromJson.InvalidJson",
                   "key '%s' not present in json",
                   key.c_str() ) ) {
    return GetStateID( config[key].asString() );
  }
  return InvalidStateID;
}

IBEIConditionPtr InternalStatesBehavior::ParseTransitionStrategy(const Json::Value& transitionConfig)
{
  const Json::Value& strategyConfig = transitionConfig["condition"];
  if( strategyConfig.isObject() ) {
    // create state concept strategy from config
    return BEIConditionFactory::CreateBEICondition(transitionConfig["condition"], GetDebugLabel());
  }
  else {
    // use code-defined named strategy
    const std::string& strategyName = JsonTools::ParseString(transitionConfig,
                                                             "preDefinedStrategyName",
                                                             "InternalStatesBehavior.StateConfig");

    auto it = _preDefinedStrategies.find(strategyName);
    if( ANKI_VERIFY( it != _preDefinedStrategies.end(),
                     "HighLevelAI.LoadTransitionConfig.PreDefinedStrategy.NotFound",
                     "No predefined strategy called '%s' exists",
                     strategyName.c_str()) ) {
      return it->second;
    }
    else {
      return nullptr;
    }
  }
}

InternalStatesBehavior::TransitionType InternalStatesBehavior::TransitionTypeFromString(const std::string& str)
{
  if( str == "NonInterrupting" ) return TransitionType::NonInterrupting;
  else if( str == "Interrupting" ) return TransitionType::Interrupting;
  else if( str == "Exit" ) return TransitionType::Exit;
  else {
    PRINT_NAMED_ERROR("InternalStatesBehavior.TransitionTypeFromString.InvalidString",
                      "String '%s' does not match a transition type",
                      str.c_str());
    // return something to make the compiler happy
    return TransitionType::Exit;
  }
}
  
std::vector<std::pair<std::string, std::vector<IBEIConditionPtr>>>
  InternalStatesBehavior::TESTONLY_GetAllTransitions( UnitTestKey key ) const
{
  std::vector<std::pair<std::string, std::vector<IBEIConditionPtr>>> ret;
  for( const auto& statePair : *_states ) {
    std::vector<IBEIConditionPtr> retTransitions;
    const State::Transitions* transitionTypes[3] = {&statePair.second._interruptingTransitions,
                                                    &statePair.second._nonInterruptingTransitions,
                                                    &statePair.second._exitTransitions};
    for( const auto& transitions : transitionTypes ) {
      for( const auto& transPair : *transitions ) {
        retTransitions.push_back( transPair.second );
      }
    }
    ret.emplace_back( statePair.second._name, std::move(retTransitions) );
  }
  return ret;
}

}
}
