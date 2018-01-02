/**
 * File: behaviorVictorObservingDemo.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-10-16
 *
 * Description: Root behavior to handle the state machine for the victor observing demo
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/victor/behaviorVictorObservingDemo.h"

#include "clad/types/needsSystemTypes.h"
#include "clad/types/objectTypes.h"
#include "coretech/common/engine/colorRGBA.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/behaviors/feeding/behaviorFeedingEat.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "engine/aiComponent/beiConditions/conditions/conditionLambda.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/faceWorld.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/needsSystem/needsState.h"
#include "engine/vision/visionModesHelpers.h"

namespace Anki {
namespace Cozmo {

namespace {

constexpr const float kSocializeKnownFaceCooldown_s = 60.0f * 10;
constexpr const float kGoToSleepTimeout_s = 60.0f * 4;
constexpr const u32 kMinFaceAgeToAllowSleep_ms = 5000;
constexpr const u32 kNeedsToChargeTime_ms = 1000 * 60 * 5;
constexpr const float kMaxFaceDistanceToSocialize_mm = 1000.0f;
constexpr const float kInitialHeadAngle_deg = 20.0f;

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
 * light 0: demo state (front)
 * light 1: green = on charger, red = cliff detected (purple = both) (middle light)
 * light 2: blinking while update is running (status light)
 */

// NOTE: current victor lights are in a wonky order
static constexpr const u32 kDebugStateLED = 0;
static constexpr const u32 kDebugRobotStatusLED = 2;
static constexpr const u32 kDebugHeartbeatLED = 1;

static constexpr const float kHeartbeatPeriod_s = 0.6f;

}


class BehaviorVictorObservingDemo::State
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
  void OnDeactivated();
  // TODO:(bn) add asserts for these

  // fills in allTransitions with the transitions present in this state
  void GetAllTransitions( std::set<IBEIConditionPtr>& allTransitions );
    
  std::string _name;
  std::string _behaviorName;
  ICozmoBehaviorPtr _behavior;

  float _lastTimeStarted_s = -1.0f;
  float _lastTimeEnded_s = -1.0f;

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

  // TODO:(bn) maybe these should be a property of the transitions or state concept strategies? That way they
  // just turn on automatically? Or at least move these into ICozmoBehavior
  std::set<VisionMode> _requiredVisionModes;

  // optional light debugging color
  ColorRGBA _debugColor = NamedColors::BLACK;
};  

////////////////////////////////////////////////////////////////////////////////

BehaviorVictorObservingDemo::BehaviorVictorObservingDemo(const Json::Value& config)
  : ICozmoBehavior(config)
  , _states(new StateMap)
  , _currDebugLights(kLightsOff)
{
  _useDebugLights = config.get("use_debug_lights", true).asBool();

  // First, parse the state definitions to create all of the "state names" as a first pass
  for( const auto& stateConfig : config[kStateConfgKey] ) {
    AddStateName( JsonTools::ParseString(stateConfig, kStateNameConfgKey, "BehaviorVictorObservingDemo.StateConfig") );
  }

  // Define hard-coded conditions
  CreatePreDefinedStrategies();
  
  // Parse the state config again to create the actual states
  for( const auto& stateConfig : config[kStateConfgKey] ) {
    State state(stateConfig);
    PRINT_CH_DEBUG("Behaviors", "VictorObservingDemo.LoadStateFromConfig",
                   "%s",
                   state._name.c_str());
    AddState(std::move(state));
  }

  DEV_ASSERT( _states->size() == _stateNameToID.size(),
              "VictorObservingDemo.StateConfig.InternalError.NotAllStatesDefined");

  ////////////////////////////////////////////////////////////////////////////////
  // Define transitions from json
  ////////////////////////////////////////////////////////////////////////////////

  // keep set of which states we've used so we can warn later if they aren't all used
  std::set< StateID > allFromStates;
  std::set< StateID > allToStates;
  
  for( const auto& transitionDefConfig : config["transitionDefinitions"] ) {
    const StateID fromStateID = ParseStateFromJson(transitionDefConfig, "from");

    if( allFromStates.find(fromStateID) != allFromStates.end() ) {
      PRINT_NAMED_WARNING("VictorObservingDemo.TransitionDefinitions.DuplicateFromState",
                          "The transition definitions have multiple blocks defining transitions from '%s'.",
                          _states->at(fromStateID)._name.c_str());
    }
    else {
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

  if( allFromStates.size() != _states->size() ) {
    PRINT_NAMED_WARNING("VictorObservingDemo.TransitionDefinitions.DeadEndStates",
                        "Some states don't have any outgoing transition strategies! %zu from states, but %zu states",
                        allFromStates.size(),
                        _states->size());
  }

  if( allToStates.size() != _states->size() ) {
    PRINT_NAMED_WARNING("VictorObservingDemo.TransitionDefinitions.UnusedStates",
                        "Some states don't have any incoming transition strategies! %zu to states, but %zu states",
                        allToStates.size(),
                        _states->size());
  }

  PRINT_CH_INFO("Behaviors", "VictorObservingDemo.StatesCreated",
                "Created %zu states",
                _states->size());
}

BehaviorVictorObservingDemo::~BehaviorVictorObservingDemo()
{
}

void BehaviorVictorObservingDemo::CreatePreDefinedStrategies()
{
  // TODO:(bn) make proper strategy for this
  _preDefinedStrategies["OnCharger"] = std::make_shared<ConditionLambda>(
    [](BehaviorExternalInterface& behaviorExternalInterface) {
      const bool onCharger = behaviorExternalInterface.GetRobotInfo().IsOnChargerPlatform();
      return onCharger;
    });

  // TODO:(bn) create a "not" strategy for this
  _preDefinedStrategies["OffCharger"] = std::make_shared<ConditionLambda>(
    [](BehaviorExternalInterface& behaviorExternalInterface) {
      const bool onCharger = behaviorExternalInterface.GetRobotInfo().IsOnChargerPlatform();
      return !onCharger;
    });


  _preDefinedStrategies["HasCubeToEat"] = std::make_shared<ConditionLambda>(
    [](BehaviorExternalInterface& behaviorExternalInterface) {
      const AIWhiteboard& whiteboard = behaviorExternalInterface.GetAIComponent().GetWhiteboard();
      return whiteboard.Victor_HasCubeToEat();
    });  

  _preDefinedStrategies["HungryAndCanEat"] = std::make_shared<ConditionLambda>(
    [](BehaviorExternalInterface& behaviorExternalInterface) {
      const AIWhiteboard& whiteboard = behaviorExternalInterface.GetAIComponent().GetWhiteboard();
      if( ! whiteboard.Victor_HasCubeToEat() ) {
        return false;
      }

      if(behaviorExternalInterface.HasNeedsManager()){
        auto& needsManager = behaviorExternalInterface.GetNeedsManager();
        NeedsState& currNeedState = needsManager.GetCurNeedsStateMutable();

        if( // currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Warning) ||
          currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical) ) {
          return true;
        }
      }
      return false;
    });

  _preDefinedStrategies["CloseFaceForSocializing"] = std::make_shared<ConditionLambda>(
    [this](BehaviorExternalInterface& behaviorExternalInterface) {
      if( !StateExitCooldownExpired(GetStateID("Socializing"), kSocializeKnownFaceCooldown_s) ) {
        // still on cooldown
        return false;
      }
      
      auto& faceWorld = behaviorExternalInterface.GetFaceWorld();
      const auto& faces = faceWorld.GetFaceIDs(true);
      for( const auto& faceID : faces ) {
        const auto* face = faceWorld.GetFace(faceID);
        if( face != nullptr ) {
          const Pose3d facePose = face->GetHeadPose();
          float distanceToFace = 0.0f;
          if( ComputeDistanceSQBetween( behaviorExternalInterface.GetRobotInfo().GetPose(),
                                        facePose,
                                        distanceToFace ) &&
              distanceToFace < Util::Square(kMaxFaceDistanceToSocialize_mm) ) {
            return true;
          }
        }
      }
      return false;
    });

  _preDefinedStrategies["WantsToSleep"] = std::make_shared<ConditionLambda>(
    [this](BehaviorExternalInterface& behaviorExternalInterface) {
      if( _currState != GetStateID("ObservingOnCharger") ) {
        PRINT_NAMED_WARNING("BehaviorVictorObservingDemo.WantsToSleepCondition.WrongState",
                            "This condition only works from ObservingOnCharger");
        return false;
      }
      
      const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      if( _states->at(GetStateID("ObservingOnCharger"))._lastTimeStarted_s + kGoToSleepTimeout_s <= currTime_s ) {

        // only go to sleep if we haven't recently seen a face
        auto& faceWorld = behaviorExternalInterface.GetFaceWorld();
        Pose3d waste;
        const bool inRobotOriginOnly = false; // might have been picked up
        const TimeStamp_t lastFaceTime = faceWorld.GetLastObservedFace(waste, inRobotOriginOnly);
        const TimeStamp_t lastImgTime = behaviorExternalInterface.GetRobotInfo().GetLastImageTimeStamp();
        if( lastFaceTime < lastImgTime &&
            lastImgTime - lastFaceTime > kMinFaceAgeToAllowSleep_ms ) {
          return true;
        }
      }
      return false;
    });

  _preDefinedStrategies["ChargerLocated"] = std::make_shared<ConditionLambda>(
    [](BehaviorExternalInterface& behaviorExternalInterface) {
      BlockWorldFilter filter;
      filter.SetFilterFcn( [](const ObservableObject* obj){
          return IsCharger(obj->GetType(), false) && obj->IsPoseStateKnown();
        });
      const auto& blockWorld = behaviorExternalInterface.GetBlockWorld();
      const auto* block = blockWorld.FindLocatedMatchingObject(filter);
      return block != nullptr;
    });

  _preDefinedStrategies["NeedsToCharge"] = std::make_shared<ConditionLambda>(
    [](BehaviorExternalInterface& behaviorExternalInterface) {
      const auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
      if( !robotInfo.IsCharging() ) {
        const TimeStamp_t lastChargeTime = robotInfo.GetLastChargingStateChangeTimestamp();
        const TimeStamp_t timeSinceNotCharging = 
          robotInfo.GetLastMsgTimestamp() > lastChargeTime ?
          robotInfo.GetLastMsgTimestamp() - lastChargeTime :
          0;

        return timeSinceNotCharging >= kNeedsToChargeTime_ms;
      }
      return false;
    });
}

void BehaviorVictorObservingDemo::InitBehavior(BehaviorExternalInterface& behaviorExternalInterface)
{
  std::set< std::shared_ptr<IBEICondition> > allTransitions;
  
  // init all of the states
  for( auto& statePair : *_states ) {
    auto& state = statePair.second;
    state.Init(behaviorExternalInterface);
    state.GetAllTransitions(allTransitions);
  }

  // initialize all transitions (from the set so they each only get initialized once)
  for( auto& strategy : allTransitions ) {
    strategy->Init(behaviorExternalInterface);
  }

  PRINT_CH_INFO("Behaviors", "VictorObservingDemo.Init",
                "initialized %zu states",
                _states->size());
}

void BehaviorVictorObservingDemo::AddState( State&& state )
{
  if( ANKI_VERIFY( GetStateID(state._name) != InvalidStateID,
                   "BehaviorVictorObservingDemo.AddState.InvalidName",
                   "State has invalid name '%s'",
                   state._name.c_str()) &&
      ANKI_VERIFY( _states->find(GetStateID(state._name)) == _states->end(),
                   "BehaviorVictorObservingDemo.AddState.StateAlreadyExists", "") ) {    
    _states->emplace(GetStateID(state._name), state);
  }
}

void BehaviorVictorObservingDemo::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  for( const auto& statePair : *_states ) {
    if( statePair.second._behavior != nullptr ) {
      delegates.insert(statePair.second._behavior.get());
    }
  }
}


void BehaviorVictorObservingDemo::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  // _visionModesToReEnable.clear();
  // auto& visionComponent = behaviorExternalInterface.GetVisionComponent();
  // for (VisionMode mode = VisionMode::Idle; mode < VisionMode::Count; ++mode) {
  //   if( visionComponent.IsModeEnabled(mode) ) {
  //     _visionModesToReEnable.push_back(mode);
  //     visionComponent.EnableMode(mode, false);
  //   }
  // }
  // Leave modes alone for now, seems to be a bug with disabled and re-enabling in the same tick

  if( _useDebugLights ) {
    // force an update
    _debugLightsDirty = true;
  }

  // start with a simple animation to make sure the eyes appear
  // TEMP: NeutralFace is currently broken, so use observing instead
  std::list<IActionRunner*> actions = {{ new TriggerAnimationAction(AnimationTrigger::ObservingLookStraight),
       new MoveHeadToAngleAction(DEG_TO_RAD(kInitialHeadAngle_deg)) }};
  CompoundActionSequential *initialAction = new CompoundActionSequential(std::move(actions));

  DelegateIfInControl(initialAction, [this](BehaviorExternalInterface& behaviorExternalInterface) {
      const bool onCharger = behaviorExternalInterface.GetRobotInfo().IsOnChargerPlatform();
      if( onCharger ) {
        TransitionToState(behaviorExternalInterface, GetStateID("ObservingOnCharger"));
      }
      else {
        TransitionToState(behaviorExternalInterface, GetStateID("Observing"));
      }
    });
  
  
}

void BehaviorVictorObservingDemo::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  TransitionToState(behaviorExternalInterface, InvalidStateID);

  auto& visionComponent = behaviorExternalInterface.GetVisionComponent();
  for( const auto& mode : _visionModesToReEnable ) {
    visionComponent.EnableMode(mode, true);
  }
  
  _visionModesToReEnable.clear();
}

void BehaviorVictorObservingDemo::BehaviorUpdate(BehaviorExternalInterface& behaviorExternalInterface)
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

    auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
    
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
      
      behaviorExternalInterface.GetBodyLightComponent().SetBackpackLights(_currDebugLights);
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

  // first check the interrupting conditions
  for( const auto& transitionPair : state._interruptingTransitions ) {
    const auto stateID = transitionPair.first;
    const auto& iConditionPtr = transitionPair.second;

    if( iConditionPtr->AreConditionsMet(behaviorExternalInterface) ) {
      TransitionToState(behaviorExternalInterface, stateID);
      return;
    }
  }

  // if we get here, then there must be no interrupting conditions that activated

  // it's ok to dispatch now if either we aren't dispatched to anything, or the behavior we are dispatched to
  // is ok with a "gentle" interruption
  bool okToDispatch = ! IsControlDelegated();

  if( !okToDispatch ) {
    if(behaviorExternalInterface.HasDelegationComponent()){
      // TODO:(bn) rather than calling this directly, we can introduce a "stop yourself in X seconds" event
      // that ICozmoBehavior implements (and hears about from StateChangeComponent)
      auto& delegationComponent = behaviorExternalInterface.GetDelegationComponent();
      const IBehavior* delegate = delegationComponent.GetBehaviorDelegatedTo(this);
      const ICozmoBehavior* delegateBehavior = dynamic_cast<const ICozmoBehavior*>(delegate);
      if( delegateBehavior ) {
        okToDispatch = delegateBehavior->CanBeGentlyInterruptedNow(behaviorExternalInterface);
      }
    }
  }

  if( okToDispatch ) {
    for( const auto& transitionPair : state._nonInterruptingTransitions ) {
      const auto stateID = transitionPair.first;
      const auto& iConditionPtr = transitionPair.second;
      
      if( iConditionPtr->AreConditionsMet(behaviorExternalInterface) ) {
        TransitionToState(behaviorExternalInterface, stateID);
        return;
      }
    }

    if( ! IsControlDelegated() ) {
      // Now there have been no interrupting or non-interrupting transitions, so check the exit conditions
      for( const auto& transitionPair : state._exitTransitions ) {
        const auto stateID = transitionPair.first;
        const auto& iConditionPtr = transitionPair.second;
      
        if( iConditionPtr->AreConditionsMet(behaviorExternalInterface) ) {
          TransitionToState(behaviorExternalInterface, stateID);
          return;
        }
      }
    }

    // if we get here, then there is no state we want to switch to, so re-start the current one if it wants to
    // run and isn't already running
    
    // TODO:(bn) can behaviors be null?
    if( !IsControlDelegated() && state._behavior->WantsToBeActivated( behaviorExternalInterface ) ) {
      DelegateIfInControl(behaviorExternalInterface,  state._behavior.get() );
    }
    // else we'll just sit here doing nothing evaluating the conditions each tick
  }

  // This demo behavior never ends, it's always running
}

void BehaviorVictorObservingDemo::TransitionToState(BehaviorExternalInterface& behaviorExternalInterface,
                                                    const StateID targetState)
{

  // TODO:(bn) don't de- and re-activate behaviors if switching states doesn't change the behavior  

  std::set<VisionMode> visionModesToDisable;
  
  if( _currState != InvalidStateID ) {
    visionModesToDisable = _states->at(_currState)._requiredVisionModes;
    _states->at(_currState).OnDeactivated();
    const bool allowCallback = false;
    CancelDelegates(allowCallback);
  }
  else {
    DEV_ASSERT( !IsControlDelegated() || targetState == InvalidStateID,
                "VictorObservingDemo.TransitionToState.WasInCountButHadDelegate" );
  }
          
  // TODO:(bn) channel for demo?
  PRINT_CH_INFO("Unfiltered", "VictorObservingDemo.TransitionToState",
                "Transition from state '%s' -> '%s'",
                _currState  != InvalidStateID ? _states->at(_currState )._name.c_str() : "<NONE>",
                targetState != InvalidStateID ? _states->at(targetState)._name.c_str() : "<NONE>");

  _currState = targetState;

  auto setVisionModes = [&behaviorExternalInterface](const std::set<VisionMode>& modes, const bool enabled) {
    for( const auto& mode : modes ) {
      auto& visionComponent = behaviorExternalInterface.GetVisionComponent();      
      if( visionComponent.IsModeEnabled(mode) != enabled ) {
        visionComponent.EnableMode(mode, enabled);
      }
    }
  };    
  
  if( _currState != InvalidStateID ) {
    State& state = _states->at(_currState);

    // don't disable any vision modes that we'll still need
    for( const auto& mode : state._requiredVisionModes ) {
      visionModesToDisable.erase(mode);
    }

    setVisionModes(visionModesToDisable, false);
    setVisionModes(state._requiredVisionModes, true);
    
    state.OnActivated(behaviorExternalInterface);

    if( _useDebugLights ) {
      if( _currDebugLights.onColors[kDebugStateLED] != state._debugColor ) {
        _debugLightsDirty = true;
        _currDebugLights.onColors[kDebugStateLED] = state._debugColor;
        _currDebugLights.offColors[kDebugStateLED] = state._debugColor;
      }
    }

    if( state._behavior->WantsToBeActivated( behaviorExternalInterface ) ) {
      DelegateIfInControl(behaviorExternalInterface,  state._behavior.get() );
    }
  }
  else {
    // disable all modes that were enabled by this state
    setVisionModes(visionModesToDisable, false);
  }
}


BehaviorVictorObservingDemo::State::State(const std::string& stateName, const std::string& behaviorName)
  : _name(stateName)
  , _behaviorName(behaviorName)
{
}

BehaviorVictorObservingDemo::State::State(const Json::Value& config)
{
  _name = JsonTools::ParseString(config, kStateNameConfgKey, "BehaviorVictorObservingDemo.StateConfig");
  _behaviorName = JsonTools::ParseString(config, "behavior", "BehaviorVictorObservingDemo.StateConfig");

  for( const auto& visionModeJson : config["visionModes"] ) {
    _requiredVisionModes.insert( VisionModeFromString( visionModeJson.asString() ) );
  }

  const std::string& debugColorStr = config.get("debugColor", "BLACK").asString();
  _debugColor = NamedColors::GetByString(debugColorStr);  
}

void BehaviorVictorObservingDemo::State::Init(BehaviorExternalInterface& bei)
{
  const auto& BC = bei.GetBehaviorContainer();
  _behavior = BC.FindBehaviorByID( BehaviorTypesWrapper::BehaviorIDFromString( _behaviorName ) );
  DEV_ASSERT_MSG(_behavior != nullptr, "ObservingDemo.State.NoBehavior",
                 "State '%s' cannot find behavior '%s'",
                 _name.c_str(),
                 _behaviorName.c_str());
}


void BehaviorVictorObservingDemo::State::GetAllTransitions( std::set<IBEIConditionPtr>& allTransitions )
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

void BehaviorVictorObservingDemo::State::AddInterruptingTransition(StateID toState,
                                                                   IBEIConditionPtr condition)
{
  // TODO:(bn) references / rvalue / avoid copies?
  _interruptingTransitions.emplace_back(toState, condition);
}

void BehaviorVictorObservingDemo::State::AddNonInterruptingTransition(StateID toState,
                                                                      IBEIConditionPtr condition )
{
  _nonInterruptingTransitions.emplace_back(toState, condition);
}

void BehaviorVictorObservingDemo::State::AddExitTransition(StateID toState, IBEIConditionPtr condition)
{
  _exitTransitions.emplace_back(toState, condition);
}

void BehaviorVictorObservingDemo::State::OnActivated(BehaviorExternalInterface& bei)
{
  for( const auto& transitionPair : _interruptingTransitions ) {
    const auto& iConditionPtr = transitionPair.second;
    iConditionPtr->Reset(bei);
  }
  for( const auto& transitionPair : _nonInterruptingTransitions ) {
    const auto& iConditionPtr = transitionPair.second;
    iConditionPtr->Reset(bei);
  }
  for( const auto& transitionPair : _exitTransitions ) {
    const auto& iConditionPtr = transitionPair.second;
    iConditionPtr->Reset(bei);
  }

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _lastTimeStarted_s = currTime_s;       
}

void BehaviorVictorObservingDemo::State::OnDeactivated()
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _lastTimeEnded_s = currTime_s;
}

bool BehaviorVictorObservingDemo::StateExitCooldownExpired(StateID state, float timeout) const
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if( _states->at(state)._lastTimeEnded_s < 0.0f ||
      _states->at(state)._lastTimeEnded_s + timeout <= currTime_s ) {
    return true;
  }
  else {
    return false;
  }
}


BehaviorVictorObservingDemo::StateID BehaviorVictorObservingDemo::AddStateName(const std::string& stateName)
{
  if( ANKI_VERIFY( _stateNameToID.find(stateName) == _stateNameToID.end(),
                   "BehaviorVictorObservingDemo.AddStateName.DuplicateName",
                   "Adding '%s' more than once",
                   stateName.c_str()) ) {
    const size_t newID = _stateNameToID.size() + 2; // +1 for the state itself, +1 to skip 0
    _stateNameToID[stateName] = newID;
    DEV_ASSERT(GetStateID(stateName) == newID, "BehaviorVictorObservingDemo.StateNameBug");
    return newID;
  }
  return InvalidStateID;
}

BehaviorVictorObservingDemo::StateID BehaviorVictorObservingDemo::GetStateID(const std::string& stateName) const
{
  auto it = _stateNameToID.find(stateName);
  if( ANKI_VERIFY( it != _stateNameToID.end(),
                   "BehaviorVictorObservingDemo.GetStateID.NoSuchState",
                   "State named '%s' does not exist",
                   stateName.c_str()) ) {
    return it->second;
  }
  return InvalidStateID;
}

BehaviorVictorObservingDemo::StateID BehaviorVictorObservingDemo::ParseStateFromJson(const Json::Value& config,
                                                                                     const std::string& key)
{
  if( ANKI_VERIFY( config[key].isString(),
                   "VictorObservingDemo.ParseStateFromJson.InvalidJson",
                   "key '%s' not present in json",
                   key.c_str() ) ) {
    return GetStateID( config[key].asString() );
  }
  return InvalidStateID;
}

IBEIConditionPtr BehaviorVictorObservingDemo::ParseTransitionStrategy(const Json::Value& transitionConfig)
{
  const Json::Value& strategyConfig = transitionConfig["condition"];
  if( strategyConfig.isObject() ) {
    // create state concept strategy from config
    return BEIConditionFactory::CreateBEICondition(transitionConfig["condition"]);
  }
  else {
    // use code-defined named strategy
    const std::string& strategyName = JsonTools::ParseString(transitionConfig,
                                                             "preDefinedStrategyName",
                                                             "BehaviorVictorObservingDemo.StateConfig");

    auto it = _preDefinedStrategies.find(strategyName);
    if( ANKI_VERIFY( it != _preDefinedStrategies.end(),
                     "VictorObservingDemo.LoadTransitionConfig.PreDefinedStrategy.NotFound",
                     "No predefined strategy called '%s' exists",
                     strategyName.c_str()) ) {
      return it->second;
    }
    else {
      return nullptr;
    }
  }
}

BehaviorVictorObservingDemo::TransitionType BehaviorVictorObservingDemo::TransitionTypeFromString(const std::string& str)
{
  if( str == "NonInterrupting" ) return TransitionType::NonInterrupting;
  else if( str == "Interrupting" ) return TransitionType::Interrupting;
  else if( str == "Exit" ) return TransitionType::Exit;
  else {
    PRINT_NAMED_ERROR("BehaviorVictorObservingDemo.TransitionTypeFromString.InvalidString",
                      "String '%s' does not match a transition type",
                      str.c_str());
    // return something to make the compiler happy
    return TransitionType::Exit;
  }
}

}
}
