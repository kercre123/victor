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

#include "clad/types/behaviorComponent/behaviorTimerTypes.h"
#include "coretech/common/engine/colorRGBA.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorTimers.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntents.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "engine/aiComponent/beiConditions/conditions/conditionLambda.h"
#include "engine/aiComponent/beiConditions/conditions/conditionTimerInRange.h"
#include "engine/aiComponent/beiConditions/conditions/conditionCarryingCube.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/unitTestKey.h"
#include "util/console/consoleFunction.h"
#include "util/console/consoleInterface.h"

#include <cctype>

namespace Anki {
namespace Vector {

CONSOLE_VAR_EXTERN(float, kTimeMultiplier);

namespace {

constexpr const char* kStateConfgKey = "states";
constexpr const char* kStateNameConfgKey = "name";
constexpr const char* kResumeReplacementsKey = "resumeReplacements";
constexpr const char* kTransitionDefinitionsKey = "transitionDefinitions";
constexpr const char* kInitialStateKey = "initialState";
constexpr const char* kStateTimerConditionsKey = "stateTimerConditions";
constexpr const char* kBeginTimeKey = "begin_s";
constexpr const char* kEmotionEventKey = "emotionEvent";
constexpr const char* kBehaviorKey = "behavior";
constexpr const char* kGetInBehaviorKey = "getInBehavior";
constexpr const char* kResetBehaviorTimerKey = "resetBehaviorTimer";
constexpr const char* kCancelSelfKey = "cancelSelf";
constexpr const char* kIgnoreMissingTransitionsKey = "ignoreMissingTransitions";

static const BackpackLightAnimation::BackpackAnimation kLightsOff = {
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

  struct Transition {
    Transition(StateID state, IBEIConditionPtr cond, const std::string& emotionEvent = "")
      : toState(state)
      , condition(cond)
      , emotionEvent(emotionEvent)
      {
      }
      
    StateID toState;
    IBEIConditionPtr condition;
    std::string emotionEvent;
  };
      
  explicit State(const Json::Value& config);

  // initialize this state after construction to fill in the behavior pointer
  void Init(BehaviorExternalInterface& bei);
  
  void AddTransition(TransitionType transType, const Transition& transition);

  void OnActivated(BehaviorExternalInterface& bei, bool isResuming);
  void OnDeactivated(BehaviorExternalInterface& bei);
  // TODO:(bn) add asserts for these

  // fills in allTransitions with the transitions present in this state
  void GetAllTransitions( std::set<IBEIConditionPtr>& allTransitions );

  // Get the time this state has been active. This timer starts when the state is activated. If there's an
  // interruption, the timer pauses rather than resetting (or continuing to track) and resumes if the state is
  // resumed.
  float GetTimeActive();
  
  std::string _name;

  // note that these also count a state as "starting" and "ending" when this behavior itself is interrupted
  // (even if we later "resume" the state)
  float _lastTimeStarted_s = -std::numeric_limits<float>::min();
  float _lastTimeEnded_s = -std::numeric_limits<float>::min();

  // This tracks an "adjusted" start time for use with GetTimeActive()
  float _adjustedStartTime_s = -std::numeric_limits<float>::min();
  
  // Transitions are evaluated in order, and if the function returns true, we will transition to the given
  // state id.
  using Transitions = std::vector<Transition>;

  using TransitionMap = std::map<TransitionType, Transitions>;

  TransitionMap _transitions;
  
  // optional light debugging color
  ColorRGBA _debugColor = NamedColors::BLACK;
  
  UserIntentTag _activateIntent = USER_INTENT(INVALID);

  std::string _behaviorName;
  ICozmoBehaviorPtr _behavior;

  std::string _getInBehaviorName;
  ICozmoBehaviorPtr _getInBehavior;

  BehaviorTimerTypes _behaviorTimer = BehaviorTimerTypes::Invalid;

};  

////////////////////////////////////////////////////////////////////////////////


InternalStatesBehavior::InternalStatesBehavior(const Json::Value& config,
                                               const CustomBEIConditionHandleList& customConditionHandles)
  : ICozmoBehavior(config, customConditionHandles)
  , _states(new StateMap)
  , _currDebugLights(kLightsOff)
{
  
  _useDebugLights = config.get("use_debug_lights", false).asBool();
  
  _ignoreMissingTransitions = config.get(kIgnoreMissingTransitionsKey, false).asBool();

  // create custom BEI conditions for timers (if any are specified)
  CustomBEIConditionHandleList customTimerHandles = CreateCustomTimerConditions(config);
  
  // First, parse the state definitions to create all of the "state names" as a first pass
  for( const auto& stateConfig : config[kStateConfgKey] ) {
    AddStateName( JsonTools::ParseString(stateConfig, kStateNameConfgKey, "InternalStatesBehavior.StateConfig") );
  }
  
  // Parse the state config again to create the actual states
  for( const auto& stateConfig : config[kStateConfgKey] ) {
    State state(stateConfig);
    PRINT_CH_DEBUG("Behaviors", "InternalStatesBehavior.LoadStateFromConfig",
                   "%s",
                   state._name.c_str());
    AddState(std::move(state));
  }

  DEV_ASSERT( _states->size() == _stateNameToID.size(),
              "InternalStatesBehavior.StateConfig.InternalError.NotAllStatesDefined");

  ////////////////////////////////////////////////////////////////////////////////
  // Define transitions from json
  ////////////////////////////////////////////////////////////////////////////////

  // keep set of which states we've used so we can warn later if they aren't all used
  std::set< StateID > allFromStates;
  std::set< StateID > allToStates;
  
  for( const auto& transitionDefConfig : config[kTransitionDefinitionsKey] ) {
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


      auto parseTransitions =
        [this, &transitionDefConfig, &allToStates, &fromState](const std::string& key,
                                                               const TransitionType transitionType) {
        
        for( const auto& transitionConfig : transitionDefConfig[key] ) {
          const StateID toState = ParseStateFromJson(transitionConfig, "to");
          IBEIConditionPtr condition = ParseTransitionStrategy(transitionConfig);
          const std::string& emotionEvent = transitionConfig.get(kEmotionEventKey, "").asString();

          fromState.AddTransition(transitionType, State::Transition{toState, condition, emotionEvent});

          allToStates.insert(toState);
        }
      };

      parseTransitions("interruptingTransitions", TransitionType::Interrupting);
      parseTransitions("nonInterruptingTransitions", TransitionType::NonInterrupting);
      parseTransitions("exitTransitions", TransitionType::Exit);
    }
  }

  if( allFromStates.size() != _states->size() ) {
    PRINT_NAMED_WARNING("InternalStatesBehavior.TransitionDefinitions.DeadEndStates",
                        "Some states don't have any outgoing transition strategies! %zu from states, but %zu states",
                        allFromStates.size(),
                        _states->size());
  }

  if( (allToStates.size() != _states->size()) && !_ignoreMissingTransitions ) {
    PRINT_NAMED_WARNING("InternalStatesBehavior.TransitionDefinitions.UnusedStates",
                        "Some states don't have any incoming transition strategies! %zu to states, but %zu states",
                        allToStates.size(),
                        _states->size());
  }

  PRINT_CH_INFO("Behaviors", "InternalStatesBehavior.StatesCreated",
                "Created %zu states",
                _states->size());

  const std::string& initialStateStr = JsonTools::ParseString(config, kInitialStateKey, "InternalStatesBehavior.StateConfig");
  auto stateIt = _stateNameToID.find(initialStateStr);
  if( ANKI_VERIFY( stateIt != _stateNameToID.end(),
                   "InternalStatesBehavior.Config.InitialState.NoSuchState",
                   "State named '%s' does not exist",
                   initialStateStr.c_str()) ) {
    _currState = stateIt->second;
    _defaultState = stateIt->second;
  }
  
  // fill out _resumeReplacements with any state replacements to be made when re-activating the behavior
  if( !config[kResumeReplacementsKey].isNull() ) {
    const auto& replacementsList = config[kResumeReplacementsKey];
    const auto& fromList = replacementsList.getMemberNames();
    for( const auto& from : fromList ) {
      StateID toId = GetStateID( replacementsList[from].asString() );
      _resumeReplacements.emplace_back( GetStateID(from), toId );
    }
  }

  // warn if any of the custom conditions aren't used
  BEIConditionFactory::CheckConditionsAreUsed(customTimerHandles, GetDebugLabel());
}
  
InternalStatesBehavior::InternalStatesBehavior(const Json::Value& config)
  : InternalStatesBehavior( config, {} )
{
}

CustomBEIConditionHandleList InternalStatesBehavior::CreateCustomTimerConditions(const Json::Value& config)
{
  CustomBEIConditionHandleList handles;

  static const char* kDebugName = "InternalStatesBehavior.CreateCustomTimerConditions";
  
  for( const auto& timerConfig : config[kStateTimerConditionsKey] ) {
    const std::string& name = JsonTools::ParseString(timerConfig, kStateNameConfgKey, kDebugName);
    const float beginTime_s = JsonTools::ParseFloat(timerConfig, kBeginTimeKey, kDebugName);
  
    handles.emplace_back(
      BEIConditionFactory::InjectCustomBEICondition(
        name,
        std::make_shared<ConditionLambda>(
          [this, beginTime_s](BehaviorExternalInterface& bei) {
            const float activatedTime = GetCurrentStateActiveTime_s();
            const float ffwdTime = kTimeMultiplier * activatedTime;
            const bool val = ffwdTime >= beginTime_s;
            return val;
          },
          GetDebugLabel() )));
  }
  
  return handles;
}

InternalStatesBehavior::~InternalStatesBehavior()
{
}
  
void InternalStatesBehavior::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kStateConfgKey,
    kResumeReplacementsKey,
    kTransitionDefinitionsKey,
    kInitialStateKey,
    kStateTimerConditionsKey,
    kEmotionEventKey,
    kIgnoreMissingTransitionsKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
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

  _putDownBlockBehavior = GetBEI().GetBehaviorContainer().FindBehaviorByID(BEHAVIOR_ID(PutDownBlock));

  // initialize all transitions (from the set so they each only get initialized once)
  for( auto& strategy : allTransitions ) {
    strategy->Init(GetBEI());
  }

  PRINT_CH_INFO("Behaviors", "InternalStatesBehavior.Init",
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
  delegates.insert(_putDownBlockBehavior.get());

  for( const auto& statePair : *_states ) {
    if( statePair.second._behavior != nullptr ) {
      delegates.insert(statePair.second._behavior.get());
    }
    if( statePair.second._getInBehavior != nullptr ) {
      delegates.insert(statePair.second._getInBehavior.get());
    }
  }
}


void InternalStatesBehavior::OnBehaviorActivated()
{
  if( _useDebugLights ) {
    // force an update
    _debugLightsDirty = true;
  }
  
  if( !_firstRun ) {
    // resuming this behavior -- see if we should be resuming in a different state than when we were
    // previously de-activated
    StateID resumeState = _currState;
    const auto it = std::find_if(_resumeReplacements.begin(), _resumeReplacements.end(), [this](const auto& p) {
      return p.first == _currState;
    });
    if( it != _resumeReplacements.end() ) {
      resumeState = it->second;
    }
    OverrideResumeState( resumeState );
    _currState = resumeState;
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
  
  _firstRun = false;
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
    if( robotInfo.GetCliffSensorComponent().IsCliffDetected() ) {
      robotStateColor.SetR(1.0f);
    }

    if( _currDebugLights.onColors[kDebugRobotStatusLED] != robotStateColor ) {
      _currDebugLights.onColors[  kDebugRobotStatusLED]  = robotStateColor;
      _currDebugLights.offColors[ kDebugRobotStatusLED]  = robotStateColor;
      
      _debugLightsDirty = true;
    }

    if( _debugLightsDirty ) {
      
      GetBEI().GetBackpackLightComponent().SetBackpackAnimation(_currDebugLights);
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

  auto tryTransition = [this](const auto& transition) {
    if( transition.condition->AreConditionsMet(GetBEI()) ) {
      if( !transition.emotionEvent.empty() ) {
        GetBEI().GetMoodManager().TriggerEmotionEvent(transition.emotionEvent);
      }      
      TransitionToState(transition.toState);
      return true;
    }
    return false;
  };

  if( _isRunningPutDownBlock ) {
    // Don't check any conditions until we've finished putting down the block
    return;
  }

  
  // first check the interrupting conditions
  for( const auto& transition : state._transitions[TransitionType::Interrupting] ) {
    if( tryTransition(transition) ) {
      return;
    }
  }

  // if we get here, then there must be no interrupting conditions that activated

  if( _isRunningGetIn ) {
    // else, the get in is still running, so don't evaluate non-interrupting conditions (or exit conditions)
    return;
  }
  
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
    for( const auto& transition : state._transitions[TransitionType::NonInterrupting] ) {
      if( tryTransition(transition) ) {
        return;
      }
    }

    if( ! IsControlDelegated() ) {
      // Now there have been no interrupting or non-interrupting transitions, so check the exit conditions
      for( const auto& transition : state._transitions[TransitionType::Exit] ) {
        if( tryTransition(transition) ) {
          return;
        }
      }
    }

    // if we get here, then there is no state we want to switch to, so re-start the current one if it wants to
    // run and isn't already running
    
    // TODO:(bn) can behaviors be null?
    if( !IsControlDelegated() ) {
      if( nullptr == state._behavior ) {
        PRINT_CH_INFO("Behaviors", "InternalStatesBehavior.BehaviorUpdate.CancelSelf",
                      "%s: in state '%s', behavior not specified, canceling self",
                      GetDebugLabel().c_str(),
                      state._name.c_str());
        CancelSelf();
      }
      else if( state._behavior->WantsToBeActivated() ) {
        DelegateIfInControl( state._behavior.get() );
      } else if( _lastTransitionTick == BaseStationTimer::getInstance()->GetTickCount() - 1 ) {
        PRINT_NAMED_WARNING( "InternalStateBehavior.BehaviorUpdate.NoTransition",
                             "There were no transitions out of state %s and the behavior %s still didn't activate!",
                             state._name.c_str(),
                             state._behaviorName.c_str() );
      }
    }
    // else we'll just sit here doing nothing evaluating the conditions each tick
  }

  // This behavior never ends, it's always running
}

void InternalStatesBehavior::TransitionToState(const StateID targetState)
{

  // TODO:(bn) don't de- and re-activate behaviors if switching states doesn't change the behavior  

  const bool isResuming = (targetState == _currState) && !_firstRun;
  
  if( _currState != InvalidStateID ) {
    _states->at(_currState).OnDeactivated(GetBEI());
    const bool allowCallback = false;
    CancelDelegates(allowCallback);
  }
  else {
    DEV_ASSERT( !IsControlDelegated() || targetState == InvalidStateID,
                "InternalStatesBehavior.TransitionToState.WasInCountButHadDelegate" );
  }
          
  // TODO:(bn) channel for high level ai?
  PRINT_CH_INFO("Unfiltered", "InternalStatesBehavior.TransitionToState",
                "Transition from state '%s' -> '%s'",
                _currState  != InvalidStateID ? _states->at(_currState )._name.c_str() : "<NONE>",
                targetState != InvalidStateID ? _states->at(targetState)._name.c_str() : "<NONE>");
  
  _currState = targetState;

  // any transition clears the "get in" that may be playing
  _isRunningGetIn = false;

  // Although state transitions shouldn't be internally driven while putting down a cube, external interrupts could
  // occur which would leave this state hung if we don't clear it deliberately
  _isRunningPutDownBlock = false;

  if( _currState != InvalidStateID ) {
    State& state = _states->at(_currState);

    state.OnActivated(GetBEI(), isResuming);

    if( _useDebugLights ) {
      if( _currDebugLights.onColors[kDebugStateLED] != state._debugColor ) {
        _debugLightsDirty = true;
        _currDebugLights.onColors[kDebugStateLED] = state._debugColor;
        _currDebugLights.offColors[kDebugStateLED] = state._debugColor;
      }
    }

    const bool canPlayGetIn = !isResuming || _firstRun;
    if( !PutDownBlockIfNecessary(state) ){
      if( !canPlayGetIn || !RunStateGetInIfAble(state)){
        RunMainStateBehavior(state);
      }
    }

    if( canPlayGetIn && state._behaviorTimer != BehaviorTimerTypes::Invalid ) {
      GetBEI().GetBehaviorTimerManager().GetTimer( state._behaviorTimer  ).Reset();
    }

    _lastTransitionTick = BaseStationTimer::getInstance()->GetTickCount();
  }
}

bool InternalStatesBehavior::PutDownBlockIfNecessary(State& state)
{
  if( _putDownBlockBehavior->WantsToBeActivated() ) {
    BehaviorOperationModifiers modifiers;
    state._behavior->GetBehaviorOperationModifiers(modifiers);
    if( !modifiers.wantsToBeActivatedWhenCarryingObject ) {
      _isRunningPutDownBlock = true;
      DelegateIfInControl(_putDownBlockBehavior.get(),
        [this, &state](){
          PRINT_CH_INFO("Behaviors", "InternalStatesBehavior.PutDownBlockBehavior.Complete",
                        "%s: in state '%s', finished PutDownBlock behavior",
                        GetDebugLabel().c_str(),
                        state._name.c_str());
          _isRunningPutDownBlock = false;
          if(!RunStateGetInIfAble(state)){
            RunMainStateBehavior(state);
          }
        });
      _isRunningPutDownBlock = true;
      return true;
    }
  }

  return false;
}

bool InternalStatesBehavior::RunStateGetInIfAble(State& state)
{
  if(state._getInBehavior != nullptr ) {
    if( state._getInBehavior->WantsToBeActivated() ) {
      _isRunningGetIn = true;
      PRINT_CH_INFO("Behaviors", "InternalStatesBehavior.GetInBehavior.Delegate",
                    "%s: transitioning into state '%s', so will delegate to get in '%s'",
                    GetDebugLabel().c_str(),
                    state._name.c_str(),
                    state._getInBehavior->GetDebugLabel().c_str());
      DelegateIfInControl(state._getInBehavior.get(),
        [this, &state](){
          PRINT_CH_INFO("Behaviors", "InternalStatesBehavior.GetInBehavior.Complete",
                        "%s: in state '%s', finished get in behavior",
                        GetDebugLabel().c_str(),
                        state._name.c_str());
          _isRunningGetIn = false;
          RunMainStateBehavior(state);
        });
      return true;
    }
    else {
      PRINT_CH_INFO("Behaviors", "InternalStatesBehavior.GetInBehavior.Skip",
                    "%s: transitioning into state '%s', but get in behavior '%s' doesn't want to activate",
                    GetDebugLabel().c_str(),
                    state._name.c_str(),
                    state._getInBehavior->GetDebugLabel().c_str());
      return false;
    }
  }

  return false;
}

void InternalStatesBehavior::RunMainStateBehavior(State& state)
{
  if( nullptr == state._behavior ) {
    PRINT_CH_INFO("Behaviors", "InternalStatesBehavior.TransitionToState.CancelSelf",
                  "%s: in state '%s', doesn't have normal behavior, canceling",
                  GetDebugLabel().c_str(),
                  state._name.c_str());
    CancelSelf();
  }
  else if( state._behavior->WantsToBeActivated() ) {
    DelegateIfInControl(state._behavior.get() );
  }
  else {
    PRINT_NAMED_WARNING( "InternalStatesBehavior.TransitionToState.NoActivation",
                          "Transitioning to state %s but behavior %s doesn't want to activate",
                          state._name.c_str(),
                          state._behaviorName.c_str() );
  }
}

float InternalStatesBehavior::GetCurrentStateActiveTime_s() const
{
  if( _currState == InvalidStateID ) {
    PRINT_NAMED_ERROR("InternalStatesBehavior.GetCurrentStateActiveTime.InvalidState",
                      "Attempted to get state time but state isn't set");
    return 0.0f;
  }

  const float activeTime = _states->at(_currState).GetTimeActive();
  return activeTime;
}

////////////////////////////////////////////////////////////////////////////////


InternalStatesBehavior::State::State(const Json::Value& config)
{
  _name = JsonTools::ParseString(config, kStateNameConfgKey, "InternalStatesBehavior.StateConfig");
  _getInBehaviorName = config.get(kGetInBehaviorKey, "").asString();

  _behaviorName = config.get(kBehaviorKey, "").asString();

  const bool cancelSelf = config.get(kCancelSelfKey, false).asBool();
  ANKI_VERIFY(cancelSelf != !_behaviorName.empty(),
              "InternalStatesBehavior.StateConfig.MissingBehavior",
              "State '%s' does not specify behavior or cancelSelf",
              _name.c_str());

  if( config[kResetBehaviorTimerKey].isString() ) {    
    _behaviorTimer = BehaviorTimerManager::BehaviorTimerFromString( config[kResetBehaviorTimerKey].asString() );
  }

  const std::string& debugColorStr = config.get("debugColor", "BLACK").asString();
  _debugColor = NamedColors::GetByString(debugColorStr);
  
  const std::string& activateIntent = config.get("activateIntent", "").asString();
  if( !activateIntent.empty() ) {
    ANKI_VERIFY( UserIntentTagFromString( activateIntent, _activateIntent ),
                 "InternalStateBehavior.State.Ctor.InvalidIntent",
                 "Could not get user intent from '%s'",
                 activateIntent.c_str() );
  }
}

void InternalStatesBehavior::State::Init(BehaviorExternalInterface& bei)
{
  const auto& BC = bei.GetBehaviorContainer();
  if( _behaviorName.empty() ) {
    _behavior = nullptr;
  }
  else {
    _behavior = BC.FindBehaviorByID( BehaviorTypesWrapper::BehaviorIDFromString( _behaviorName ) );
    DEV_ASSERT_MSG(_behavior != nullptr, "InternalStatesBehavior.State.NoBehavior",
                   "State '%s' cannot find behavior '%s'",
                   _name.c_str(),
                   _behaviorName.c_str());
  }
  
  if( !_getInBehaviorName.empty() ) {
    _getInBehavior = BC.FindBehaviorByID( BehaviorTypesWrapper::BehaviorIDFromString( _getInBehaviorName ) );
    DEV_ASSERT_MSG(_getInBehavior != nullptr, "InternalStatesBehavior.State.NoGetInBehavior",
                   "State '%s' cannot find getInBehavior '%s'",
                   _name.c_str(),
                   _getInBehaviorName.c_str());

    PRINT_CH_INFO("Behaviors", "InternalStatesBehavior.GetInBehavior.Defined",
                  "state '%s' found behavior %p matching name '%s'",
                  _name.c_str(),
                  _getInBehavior.get(),
                  _getInBehaviorName.c_str());
  }
}


void InternalStatesBehavior::State::GetAllTransitions( std::set<IBEIConditionPtr>& allTransitions )
{
  for( const auto& transitionTypes : _transitions ) {
    for( const auto& transition : transitionTypes.second ) {
      allTransitions.insert(transition.condition);
    }
  }
}

void InternalStatesBehavior::State::AddTransition(TransitionType transType, const Transition& transition)
{
  _transitions[transType].emplace_back(transition);
}

void InternalStatesBehavior::State::OnActivated(BehaviorExternalInterface& bei, bool isResuming)
{
  if( _activateIntent != USER_INTENT(INVALID) ) {
    auto& uic = bei.GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();
    if( uic.IsUserIntentPending( _activateIntent ) ) {
      uic.ActivateUserIntent( _activateIntent, ("InternalState:" + _name) );
    }
  }

  for( const auto& transitionTypes : _transitions ) {
    for( const auto& transition : transitionTypes.second ) {
      transition.condition->SetActive(bei, true);
    }
  }

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _lastTimeStarted_s = currTime_s;

  const bool adjustedTimeNeverSet = ( _adjustedStartTime_s <= 0.0f );
  if( !isResuming || adjustedTimeNeverSet ) {
    // initial activation of this state
    _adjustedStartTime_s = currTime_s;
  }
  else {
    // this is a "resume" of the state, so subtract the time that this was "paused" (due to this behavior
    // itself being interrupted)
    const float pausedFor_s = currTime_s - _lastTimeEnded_s;
    if( ANKI_VERIFY( pausedFor_s >= 0.0f,
                     "InternalStatesBehavior.State.OnActivated.AdjustedTimeInvalid",
                     "State '%s' looks like it's been paused for %f s (%f - %f)",
                     _name.c_str(),
                     pausedFor_s,
                     currTime_s,
                     _lastTimeEnded_s ) ) {
      
      _adjustedStartTime_s += pausedFor_s;

      PRINT_CH_DEBUG("Behaviors", "InternalStatesBehavior.State.PausedFor",
                     "State '%s' was paused for %fs",
                     _name.c_str(),
                     pausedFor_s);
    }
  }
}

void InternalStatesBehavior::State::OnDeactivated(BehaviorExternalInterface& bei)
{
  for( const auto& transitionTypes : _transitions ) {
    for( const auto& transition : transitionTypes.second ) {
      transition.condition->SetActive(bei, false);
    }
  }

  if( _activateIntent != USER_INTENT(INVALID) ) {
    auto& uic = bei.GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();
    if( uic.IsUserIntentActive( _activateIntent ) ) {
      uic.DeactivateUserIntent( _activateIntent );
    }
  }

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _lastTimeEnded_s = currTime_s;
}

float InternalStatesBehavior::State::GetTimeActive()
{
  const bool adjustedTimeNeverSet = ( _adjustedStartTime_s == -std::numeric_limits<float>::min() );
  if( adjustedTimeNeverSet ) {
    return 0.0f;
  }

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const float returnTime = currTime_s - _adjustedStartTime_s;
  DEV_ASSERT( Util::IsFltGE(returnTime, 0.0f), "InternalStatesBehavior.State.AdjustedTime.NegativeTimeBug" );
  return returnTime;
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

const std::string& InternalStatesBehavior::GetStateName(const StateID state)
{
  if( state == InternalStatesBehavior::InvalidStateID ) {
    static const std::string invalidStr = "<INVALID>";
    return invalidStr;
  }
  
  const auto it = _states->find(state);
  if( it == _states->end() ) {
    PRINT_NAMED_ERROR("InternalStatesBehavior.GetStateName.InvalidState",
                      "Invalid state %zu",
                      state);
    static const std::string empty;
    return empty;
  }

  return it->second._name;
}

float InternalStatesBehavior::GetLastTimeStarted(StateID state) const
{
  return _states->at(state)._lastTimeStarted_s;
}
  
float InternalStatesBehavior::GetLastTimeEnded(StateID state) const
{
  return _states->at(state)._lastTimeEnded_s;
}
  
InternalStatesBehavior::StateID InternalStatesBehavior::ParseStateFromJson(const Json::Value& config,
                                                                           const std::string& key)
{
  if( ANKI_VERIFY( config[key].isString(),
                   "InternalStatesBehavior.ParseStateFromJson.InvalidJson",
                   "key '%s' not present in json",
                   key.c_str() ) ) {
    return GetStateID( config[key].asString() );
  }
  else {
    JsonTools::PrintJsonError(config, "InternalStatesBehavior.ParseStateFromJson.InvalidJson", 2);
  }
  return InvalidStateID;
}

IBEIConditionPtr InternalStatesBehavior::ParseTransitionStrategy(const Json::Value& transitionConfig)
{
  const Json::Value& strategyConfig = transitionConfig["condition"];
  if( ANKI_VERIFY( strategyConfig.isObject(), "InternalStatesBehavior.ParseTransitionStrategy.NoCondition",
                   "Behavior '%s' missing condition config in a transition",
                   GetDebugLabel().c_str())) {
    // create state concept strategy from config
    return BEIConditionFactory::CreateBEICondition(transitionConfig["condition"], GetDebugLabel());
  }
  else {
    return nullptr;
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

    for( const auto& transitionTypes : statePair.second._transitions ) {
      for( const auto& transition : transitionTypes.second ) {
        retTransitions.push_back( transition.condition );
      }
    }

    ret.emplace_back( statePair.second._name, std::move(retTransitions) );
  }
  return ret;
}
  
bool InternalStatesBehavior::TESTONLY_IsStateRunning( UnitTestKey key, const std::string& name ) const
{
  return (GetCurrentStateID() != InvalidStateID)
      && (GetCurrentStateID() == GetStateID(name));
}

}
}
