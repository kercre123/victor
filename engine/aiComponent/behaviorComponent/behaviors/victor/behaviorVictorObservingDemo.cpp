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
#include "coretech/common/include/anki/common/basestation/colorRGBA.h"
#include "coretech/common/include/anki/common/basestation/jsonTools.h"
#include "coretech/common/include/anki/common/basestation/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/behaviors/feeding/behaviorFeedingEat.h"
#include "engine/aiComponent/stateConceptStrategies/iStateConceptStrategy.h"
#include "engine/aiComponent/stateConceptStrategies/strategyLambda.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/faceWorld.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/needsSystem/needsState.h"
#include "engine/vision/visionModesHelpers.h"

// TODO:(bn) remove this after data-driving
#include "engine/aiComponent/stateConceptStrategies/strategyAlwaysRun.h"

namespace Anki {
namespace Cozmo {

namespace {

constexpr const float kFeedingTimeout_s = 30.0f;
constexpr const float kRecentlyPlacedChargerTimeout_s = 60.0f * 2;
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

// TODO:(bn) move somewhere else
class TimerStrategy : public IStateConceptStrategy
{
public:
  TimerStrategy(const float timeout_s);

  virtual void ResetInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual bool AreStateConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

private:
  const float _timeout_s;
  float _timeToEnd_s = -1.0f;
};

TimerStrategy::TimerStrategy(const float timeout_s)
  : IStateConceptStrategy(IStateConceptStrategy::GenerateBaseStrategyConfig(StateConceptStrategyType::Timer))
  , _timeout_s(timeout_s)
{
}

void TimerStrategy::ResetInternal(BehaviorExternalInterface& bei)
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _timeToEnd_s = currTime_s + _timeout_s;
}

bool TimerStrategy::AreStateConditionsMetInternal(BehaviorExternalInterface& bei) const
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  return currTime_s >= _timeToEnd_s;
}

}

class BehaviorVictorObservingDemo::State
{
public:
  
  State(const std::string& stateName, const std::string& behaviorName);
  explicit State(const Json::Value& config);

  // initialize this state after construction to fill in the behavior pointer
  void Init(BehaviorExternalInterface& bei);
  
  void AddInterruptingTransition(StateID toState, IStateConceptStrategyPtr condition );
  void AddNonInterruptingTransition(StateID toState, IStateConceptStrategyPtr condition );
  void AddExitTransition(StateID toState, IStateConceptStrategyPtr condition);

  void OnActivated(BehaviorExternalInterface& bei);
  void OnDeactivated();
  // TODO:(bn) add asserts for these

  // fills in allTransitions with the transitions present in this state
  void GetAllTransitions( std::set<IStateConceptStrategyPtr>& allTransitions );
    
  std::string _name;
  std::string _behaviorName;
  ICozmoBehaviorPtr _behavior;

  float _lastTimeStarted_s = -1.0f;
  float _lastTimeEnded_s = -1.0f;

  // Transitions are evaluated in order, and if the function returns true, we will transition to the given
  // state id.
  using Transitions = std::vector< std::pair< StateID, IStateConceptStrategyPtr > >;

  // transitions that can happen while the state is active (and in the middle of doing something)
  Transitions _interruptingTransitions;

  // transitions that can happen when the currently delegated-to behavior thinks it's a decent time for a
  // gentle interruption (or if there is no currently delegated behavior)
  Transitions _nonInterruptingTransitions;

  // exit transitions only run if the currently-delegated-to behavior stop itself. Note that these are
  // checked _after_ all of the other transitions
  Transitions _exitTransitions;

  // TODO:(bn) maybe these should be a property of ICondition? That way they just turn on automatically?
  // TODO:(bn) ICozmoBehavior would actually be the better place for this (or a sub-component of ICozmoBehavior)
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

  // phase 0: fake loading in the "state names" in a first pass. Need to do this now to be able to use them in
  // the conditions
  
  for( const auto& stateConfig : config[kStateConfgKey] ) {
    AddStateName( JsonTools::ParseString(stateConfig, kStateNameConfgKey, "BehaviorVictorObservingDemo.StateConfig") );
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Define conditions
  ////////////////////////////////////////////////////////////////////////////////

  auto isHungryLambda = [](BehaviorExternalInterface& behaviorExternalInterface) {
    if(behaviorExternalInterface.HasNeedsManager()){
      auto& needsManager = behaviorExternalInterface.GetNeedsManager();
      NeedsState& currNeedState = needsManager.GetCurNeedsStateMutable();

      if( // currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Warning) ||
        currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical) ) {
        return true;
      }
    }
    return false;
  };
    
  auto CondIsHungry = std::make_shared<StrategyLambda>(isHungryLambda);

  auto CondOnCharger = std::make_shared<StrategyLambda>(
    [](BehaviorExternalInterface& behaviorExternalInterface) {
      const bool onCharger = behaviorExternalInterface.GetRobotInfo().IsOnChargerPlatform();
      return onCharger;
    });

  auto CondOffCharger = std::make_shared<StrategyLambda>(
    [](BehaviorExternalInterface& behaviorExternalInterface) {
      const bool onCharger = behaviorExternalInterface.GetRobotInfo().IsOnChargerPlatform();
      return !onCharger;
    });

  auto canEatLambda = [](BehaviorExternalInterface& behaviorExternalInterface) {
    const AIWhiteboard& whiteboard = behaviorExternalInterface.GetAIComponent().GetWhiteboard();
    return whiteboard.Victor_HasCubeToEat();
  };
  
  auto CondCanEat = std::make_shared<StrategyLambda>( canEatLambda );

  auto CondHungryAndCanEat = std::make_shared<StrategyLambda>(
    [canEatLambda, isHungryLambda](BehaviorExternalInterface& behaviorExternalInterface) {
      return canEatLambda(behaviorExternalInterface) && isHungryLambda(behaviorExternalInterface);
    });
  
  // TEMP:  // TODO:(bn) rename this to "true" since "run" doesn't make sense anymore
  auto CondTrue = std::make_shared<StrategyAlwaysRun>(
    IStateConceptStrategy::GenerateBaseStrategyConfig(StateConceptStrategyType::AlwaysRun));

  auto CondCloseFaceForSocializing = std::make_shared<StrategyLambda>(
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

  auto CondWantsToSleep = std::make_shared<StrategyLambda>(
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

  auto CondSeesCharger = std::make_shared<StrategyLambda>(
    [](BehaviorExternalInterface& behaviorExternalInterface) {
      BlockWorldFilter filter;
      filter.SetFilterFcn( [](const ObservableObject* obj){
          return IsCharger(obj->GetType(), false) && obj->IsPoseStateKnown();
        });
      const auto& blockWorld = behaviorExternalInterface.GetBlockWorld();
      const auto* block = blockWorld.FindLocatedMatchingObject(filter);
      return block != nullptr;
    });

  auto CondNeedsToCharge = std::make_shared<StrategyLambda>(
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
  
  ////////////////////////////////////////////////////////////////////////////////
  // Define from json
  ////////////////////////////////////////////////////////////////////////////////

  for( const auto& stateConfig : config[kStateConfgKey] ) {
    State state(stateConfig);
    PRINT_CH_DEBUG("Behaviors", "VictorObservingDemo.LoadStateFromConfig",
                   "%s",
                   state._name.c_str());
    AddState(std::move(state));
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Define transitions // TODO:(bn) move these to json also
  ////////////////////////////////////////////////////////////////////////////////

  {
    State& state = _states->at(GetStateID("ObservingOnCharger"));
    
    state.AddNonInterruptingTransition(GetStateID("DriveOffChargerIntoObserving"), CondIsHungry);
    state.AddInterruptingTransition(GetStateID("Observing"), CondOffCharger);
    state.AddInterruptingTransition(GetStateID("DriveOffChargerIntoFeeding"), CondHungryAndCanEat);
    state.AddInterruptingTransition(GetStateID("DriveOffChargerIntoSocializing"), CondCloseFaceForSocializing);
    state.AddNonInterruptingTransition(GetStateID("Napping"), CondWantsToSleep);
  }

  {
    State& state = _states->at(GetStateID("ObservingOnChargerRecentlyPlaced"));

    state.AddInterruptingTransition(GetStateID("Observing"), CondOffCharger);
    state.AddNonInterruptingTransition(GetStateID("ObservingOnCharger"),
                                       std::make_shared<TimerStrategy>(kRecentlyPlacedChargerTimeout_s));
    state.AddNonInterruptingTransition(GetStateID("DriveOffChargerIntoFeeding"), CondHungryAndCanEat);
  }

  {
    State& state = _states->at(GetStateID("DriveOffChargerIntoObserving"));

    state.AddExitTransition(GetStateID("Socializing"), CondCloseFaceForSocializing);
    state.AddExitTransition(GetStateID("Observing"), CondTrue);
  }


  {
    State& state = _states->at(GetStateID("DriveOffChargerIntoFeeding"));
    
    state.AddExitTransition(GetStateID("Feeding"), CondCanEat);
    state.AddExitTransition(GetStateID("Observing"), CondTrue);
  }

  {
    State& state = _states->at(GetStateID("DriveOffChargerIntoSocializing"));
    
    state.AddExitTransition(GetStateID("Socializing"), CondTrue);
  }

  {
    State& state = _states->at(GetStateID("Observing"));

    state.AddInterruptingTransition(GetStateID("ObservingOnChargerRecentlyPlaced"), CondOnCharger);
    state.AddInterruptingTransition(GetStateID("Feeding"), CondCanEat);
    state.AddInterruptingTransition(GetStateID("Socializing"), CondCloseFaceForSocializing);
    state.AddNonInterruptingTransition(GetStateID("ReturningToCharger"), CondNeedsToCharge);
  }

  {
    State& state = _states->at(GetStateID("Feeding"));

    state.AddNonInterruptingTransition(GetStateID("Observing"), std::make_shared<TimerStrategy>(kFeedingTimeout_s));
    state.AddInterruptingTransition(GetStateID("ObservingOnChargerRecentlyPlaced"), CondOnCharger);
    state.AddExitTransition(GetStateID("Observing"), CondTrue);
  }

  {
    State& state = _states->at(GetStateID("Socializing"));

    state.AddInterruptingTransition(GetStateID("ObservingOnChargerRecentlyPlaced"), CondOnCharger);
    state.AddNonInterruptingTransition(GetStateID("Feeding"), CondHungryAndCanEat);
    state.AddExitTransition(GetStateID("Observing"), CondTrue);
  }

  {
    State& state = _states->at(GetStateID("Napping"));

    state.AddInterruptingTransition(GetStateID("WakingUp"), CondOffCharger);
    state.AddNonInterruptingTransition(GetStateID("WakingUp"), CondIsHungry);
  }

  {
    State& state = _states->at(GetStateID("WakingUp"));
    
    state.AddExitTransition(GetStateID("ObservingOnCharger"), CondTrue);
  }

  {
    State& state = _states->at(GetStateID("ReturningToCharger"));

    // use "recently placed" so he stays on long enough to charge up a bit. May need a separate state for that
    // at some point
    state.AddExitTransition(GetStateID("ObservingOnChargerRecentlyPlaced"), CondOnCharger);
    state.AddExitTransition(GetStateID("FailedToFindCharger"), CondTrue);
    // TODO:(bn) this doesn't work right now because FindAndGoToHome never stops unless it find a charger
  }

  {
    State& state = _states->at(GetStateID("FailedToFindCharger"));

    state.AddInterruptingTransition(GetStateID("ReturningToCharger"), CondSeesCharger);
    state.AddExitTransition(GetStateID("ObservingOnCharger"), CondOnCharger);
  }

  PRINT_CH_INFO("Behaviors", "VictorObservingDemo.StatesCreated",
                "Created %zu states",
                _states->size());

  // TODO:(bn) assert that all states are present in the map
}

BehaviorVictorObservingDemo::~BehaviorVictorObservingDemo()
{
}

void BehaviorVictorObservingDemo::InitBehavior(BehaviorExternalInterface& behaviorExternalInterface)
{
  std::set< std::shared_ptr<IStateConceptStrategy> > allTransitions;
  
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


Result BehaviorVictorObservingDemo::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
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
  
  return Result::RESULT_OK;
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

ICozmoBehavior::Status BehaviorVictorObservingDemo::UpdateInternal_WhileRunning(
  BehaviorExternalInterface& behaviorExternalInterface)
{

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
    return ICozmoBehavior::UpdateInternal_WhileRunning(behaviorExternalInterface);
  }
  
  State& state = _states->at(_currState);

  // first check the interrupting conditions
  for( const auto& transitionPair : state._interruptingTransitions ) {
    const auto stateID = transitionPair.first;
    const auto& iConditionPtr = transitionPair.second;

    if( iConditionPtr->AreStateConditionsMet(behaviorExternalInterface) ) {
      TransitionToState(behaviorExternalInterface, stateID);
      return Status::Running;
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
      
      if( iConditionPtr->AreStateConditionsMet(behaviorExternalInterface) ) {
        TransitionToState(behaviorExternalInterface, stateID);
        return Status::Running;
      }
    }

    if( ! IsControlDelegated() ) {
      // Now there have been no interrupting or non-interrupting transitions, so check the exit conditions
      for( const auto& transitionPair : state._exitTransitions ) {
        const auto stateID = transitionPair.first;
        const auto& iConditionPtr = transitionPair.second;
      
        if( iConditionPtr->AreStateConditionsMet(behaviorExternalInterface) ) {
          TransitionToState(behaviorExternalInterface, stateID);
          return Status::Running;
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
  return Status::Running;
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
    DEV_ASSERT( !IsControlDelegated(), "VictorObservingDemo.TransitionToState.WasInCountButHadDelegate" );
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


void BehaviorVictorObservingDemo::State::GetAllTransitions( std::set<IStateConceptStrategyPtr>& allTransitions )
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
                                                                   IStateConceptStrategyPtr condition)
{
  // TODO:(bn) references / rvalue / avoid copies?
  _interruptingTransitions.emplace_back(toState, condition);
}

void BehaviorVictorObservingDemo::State::AddNonInterruptingTransition(StateID toState,
                                                                      IStateConceptStrategyPtr condition )
{
  _nonInterruptingTransitions.emplace_back(toState, condition);
}

void BehaviorVictorObservingDemo::State::AddExitTransition(StateID toState, IStateConceptStrategyPtr condition)
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

}
}
