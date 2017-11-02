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

#include "clad/types/behaviorComponent/behaviorTypes.h"
#include "clad/types/needsSystemTypes.h"
#include "coretech/common/include/anki/common/basestation/utils/timer.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorListenerInterfaces/iFeedingListener.h"
#include "engine/aiComponent/behaviorComponent/behaviors/feeding/behaviorFeedingEat.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/faceWorld.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/needsSystem/needsState.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

namespace {

static constexpr const float kFeedingTimeout_s = 30.0f;
static constexpr const float kRecentlyPlacedChargerTimeout_s = 60.0f * 0.2f; // 10; // TEMP: 
static constexpr const float kSocializeKnownFaceCooldown = 60.0f * 30;
static constexpr const float kGoToSleepTimeout_s = 60.0f * 0.5f; // TEMP: much much longer
static constexpr const float kWantsToPlayTimeout_s = 60.0f * 20;
static constexpr const u32 kMinFaceAgeToAllowSleep_ms = 5000;

// TODO:(bn) move somewhere else
class TimeoutCondition : public ICondition
{
public:
  TimeoutCondition(const float timeout_s);

  virtual void EnteredScope() override;
  virtual bool Evaluate(BehaviorExternalInterface& behaviorExternalInterface) override;

private:
  const float _timeout_s;
  float _timeToEnd_s = -1.0f;
};

TimeoutCondition::TimeoutCondition(const float timeout_s)
  : _timeout_s(timeout_s)
{
}

void TimeoutCondition::EnteredScope()
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _timeToEnd_s = currTime_s + _timeout_s;
}

bool TimeoutCondition::Evaluate(BehaviorExternalInterface& behaviorExternalInterface)
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  return currTime_s >= _timeToEnd_s;
}

}


// TODO:(bn) move this
class FeedingListenerCondition : public ICondition, public IFeedingListener
{
public:

  FeedingListenerCondition(std::shared_ptr<BehaviorFeedingEat> feedingBehavior) : _behavior(feedingBehavior) {
    _behavior->AddListener(this);
  }
  virtual ~FeedingListenerCondition() { if( _behavior ) { _behavior->RemoveListeners(this); } }
  
  // Implementation of IFeedingListener
  virtual void StartedEating(BehaviorExternalInterface& behaviorExternalInterface, const int duration_s) override { _complete = false; }
  virtual void EatingComplete(BehaviorExternalInterface& behaviorExternalInterface) override { _complete = true; }
  virtual void EatingInterrupted(BehaviorExternalInterface& behaviorExternalInterface) override { _complete = true; }

  virtual bool Evaluate(BehaviorExternalInterface& behaviorExternalInterface) override { return _complete; }

private:

  bool _complete = false;
  std::shared_ptr<BehaviorFeedingEat> _behavior = nullptr;
};

class TrueCondition : public ICondition
{
public:
  virtual bool Evaluate(BehaviorExternalInterface& behaviorExternalInterface) override { return true; }
};
  

////////////////////////////////////////////////////////////////////////////////

BehaviorVictorObservingDemo::BehaviorVictorObservingDemo(const Json::Value& config)
  : ICozmoBehavior(config)
{
}

void BehaviorVictorObservingDemo::InitBehavior(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEV_ASSERT(!_initComplete, "BehaviorVictorObservingDemo.AlreadyInited");
  _initComplete = false;

  ////////////////////////////////////////////////////////////////////////////////
  // Define conditions
  ////////////////////////////////////////////////////////////////////////////////

  auto CondIsHungry = std::make_shared<LambdaCondition>(
    [](BehaviorExternalInterface& behaviorExternalInterface) {
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

  auto CondOnCharger = std::make_shared<LambdaCondition>(
    [](BehaviorExternalInterface& behaviorExternalInterface) {
      const bool onCharger = behaviorExternalInterface.GetRobot().IsOnChargerPlatform();
      return onCharger;
    });

  auto CondOffCharger = std::make_shared<LambdaCondition>(
    [](BehaviorExternalInterface& behaviorExternalInterface) {
      const bool onCharger = behaviorExternalInterface.GetRobot().IsOnChargerPlatform();
      return !onCharger;
    });

  auto CondCanEat = std::make_shared<LambdaCondition>(
    [](BehaviorExternalInterface& behaviorExternalInterface) {
      const AIWhiteboard& whiteboard = behaviorExternalInterface.GetAIComponent().GetWhiteboard();
      return whiteboard.Victor_HasCubeToEat();
    });

  {
    // the feeding complete condition is special because it needs to stay around to be a "listener"a
    auto& BC = behaviorExternalInterface.GetBehaviorContainer();
    std::shared_ptr<BehaviorFeedingEat> eatingBehavior;
    const bool foundBehavior = BC.FindBehaviorByIDAndDowncast(BehaviorID::FeedingEat,
                                                              BehaviorClass::FeedingEat,
                                                              eatingBehavior);
    if( ANKI_VERIFY(foundBehavior,
                    "VictorObservingDemo.NoEatingBehavior",
                    "couldn't find and downcast eating behavior" ) ) {
      _feedingCompleteCondition.reset( new FeedingListenerCondition( eatingBehavior ) );
    }
  }

  auto CondTrue = std::make_shared<TrueCondition>();

  auto CondNewKnownFace = std::make_shared<LambdaCondition>(
    [this](BehaviorExternalInterface& behaviorExternalInterface) {
      auto& faceWorld = behaviorExternalInterface.GetFaceWorld();
      const auto& faces = faceWorld.GetFaceIDs(true);
      for( const auto& faceID : faces ) {
        const auto* face = faceWorld.GetFace(faceID);
        if( face != nullptr && face->HasName() ) {
          if( StateExitCooldownExpired(StateID::Socializing, kSocializeKnownFaceCooldown) ) {
            return true;
          }
        }
      }
      return false;
    });

  auto CondWantsToPlay = std::make_shared<LambdaCondition>(
    [this](BehaviorExternalInterface& behaviorExternalInterface) {
      if( StateExitCooldownExpired(StateID::Playing, kWantsToPlayTimeout_s) ) {
        BlockWorldFilter filter;
        filter.SetAllowedTypes( {{ ObjectType::Block_LIGHTCUBE2, ObjectType::Block_LIGHTCUBE3 }} );
        filter.SetFilterFcn( [](const ObservableObject* obj){ return obj->IsPoseStateKnown(); });
        
        const auto& blockWorld = behaviorExternalInterface.GetBlockWorld();
        const auto* block = blockWorld.FindLocatedMatchingObject(filter);
        if( block != nullptr ) {
          return true;
        }
      }
      return false;
    });

  auto CondWantsToSleep = std::make_shared<LambdaCondition>(
    [this](BehaviorExternalInterface& behaviorExternalInterface) {
      if( _currState != StateID::ObservingOnCharger ) {
        PRINT_NAMED_WARNING("BehaviorVictorObservingDemo.WantsToSleepCondition.WrongState",
                            "This condition only works from ObservingOnCharger");
        return false;
      }
      
      const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      if( _states.at(StateID::ObservingOnCharger)._lastTimeStarted_s + kGoToSleepTimeout_s <= currTime_s ) {

        // only go to sleep if we haven't recently seen a face
        auto& faceWorld = behaviorExternalInterface.GetFaceWorld();
        Pose3d waste;
        const bool inRobotOriginOnly = false; // might have been picked up
        const TimeStamp_t lastFaceTime = faceWorld.GetLastObservedFace(waste, inRobotOriginOnly);
        const TimeStamp_t lastImgTime = behaviorExternalInterface.GetRobot().GetLastImageTimeStamp();
        if( lastFaceTime < lastImgTime &&
            lastImgTime - lastFaceTime > kMinFaceAgeToAllowSleep_ms ) {
          return true;
        }
      }
      return false;
    });
    
  
  ////////////////////////////////////////////////////////////////////////////////
  // Define states
  ////////////////////////////////////////////////////////////////////////////////

  const auto& BC = behaviorExternalInterface.GetBehaviorContainer();

  {
    ICozmoBehaviorPtr behavior = BC.FindBehaviorByID(BehaviorID::VictorDemoObservingOnChargerState);
    DEV_ASSERT(behavior != nullptr, "ObservingDemo.NoBehavior.VictorDemoObservingOnChargerState");

    State state(StateID::ObservingOnCharger, behavior);
    state.AddNonInterruptingTransition(StateID::DriveOffChargerIntoObserving, CondIsHungry);
    state.AddInterruptingTransition(StateID::Observing, CondOffCharger);
    state.AddInterruptingTransition(StateID::Feeding, CondCanEat);
    state.AddInterruptingTransition(StateID::Socializing, CondNewKnownFace);
    state.AddNonInterruptingTransition(StateID::Napping, CondWantsToSleep);
    // state.AddNonInterruptingTransition(StateID::DriveOffChargerIntoPlay, CondWantsToPlay);
    AddState(std::move(state));
  }

  {
    ICozmoBehaviorPtr behavior = BC.FindBehaviorByID(BehaviorID::VictorDemoObservingOnChargerState);
    DEV_ASSERT(behavior != nullptr, "ObservingDemo.NoBehavior.VictorDemoObservingOnChargerState");

    State state(StateID::ObservingOnChargerRecentlyPlaced, behavior);
    state.AddInterruptingTransition(StateID::Observing, CondOffCharger);
    state.AddNonInterruptingTransition(StateID::ObservingOnCharger,
                                       std::make_shared<TimeoutCondition>(kRecentlyPlacedChargerTimeout_s));
    AddState(std::move(state));
  }

  {
    ICozmoBehaviorPtr behavior = BC.FindBehaviorByID(BehaviorID::VictorDemoDriveOffChargerIntoObserving);
    DEV_ASSERT(behavior != nullptr, "ObservingDemo.NoBehavior.VictorDemoObservingDriveOffCharger");

    State driveOffCharger(StateID::DriveOffChargerIntoObserving, behavior);
    driveOffCharger.AddNonInterruptingTransition(StateID::Socializing, CondNewKnownFace);
    driveOffCharger.AddExitTransition(StateID::Observing, CondTrue);
    AddState(std::move(driveOffCharger));
  }

  {
    ICozmoBehaviorPtr behavior = BC.FindBehaviorByID(BehaviorID::DriveOffCharger);
    DEV_ASSERT(behavior != nullptr, "ObservingDemo.NoBehavior.DriveOffCharger");

    State driveOffCharger(StateID::DriveOffChargerIntoPlay, behavior);
    driveOffCharger.AddExitTransition(StateID::Playing, CondTrue);
    AddState(std::move(driveOffCharger));
  }

  {
    ICozmoBehaviorPtr behavior = BC.FindBehaviorByID(BehaviorID::VictorDemoObservingState);
    DEV_ASSERT(behavior != nullptr, "ObservingDemo.NoBehavior.VictorDemoObservingState");

    State state(StateID::Observing, behavior);
    state.AddInterruptingTransition(StateID::ObservingOnChargerRecentlyPlaced, CondOnCharger);
    state.AddInterruptingTransition(StateID::Feeding, CondCanEat);
    state.AddInterruptingTransition(StateID::Socializing, CondNewKnownFace);
    // state.AddNonInterruptingTransition(StateID::Playing, CondWantsToPlay);
    AddState(std::move(state));
  }

  {
    ICozmoBehaviorPtr behavior = BC.FindBehaviorByID(BehaviorID::VictorDemoFeedingState);
    DEV_ASSERT(behavior != nullptr, "ObservingDemo.NoBehavior.VictorDemoFeedingState");

    State state(StateID::Feeding, behavior);
    state.AddNonInterruptingTransition(StateID::Observing, std::make_shared<TimeoutCondition>(kFeedingTimeout_s));
    state.AddInterruptingTransition(StateID::ObservingOnChargerRecentlyPlaced, CondOnCharger);

    if( _feedingCompleteCondition ) {
      state.AddNonInterruptingTransition(StateID::Observing, _feedingCompleteCondition);
    }        
    
    AddState(std::move(state));
  }

  {
    ICozmoBehaviorPtr behavior = BC.FindBehaviorByID(BehaviorID::VictorDemoSocialize);
    DEV_ASSERT(behavior != nullptr, "ObservingDemo.NoBehavior.VictorDemoSocialize");

    State state(StateID::Socializing, behavior);
    state.AddInterruptingTransition(StateID::ObservingOnChargerRecentlyPlaced, CondOnCharger);
    state.AddNonInterruptingTransition(StateID::Feeding, CondCanEat);
    state.AddExitTransition(StateID::Observing, CondTrue);
    
    AddState(std::move(state));
  }

  {
    ICozmoBehaviorPtr behavior = BC.FindBehaviorByID(BehaviorID::VictorDemoPlayState);
    DEV_ASSERT(behavior != nullptr, "ObservingDemo.NoBehavior.VictorDemoPlayState");

    State state(StateID::Playing, behavior);
    state.AddInterruptingTransition(StateID::ObservingOnChargerRecentlyPlaced, CondOnCharger);
    state.AddExitTransition(StateID::Observing, CondTrue);
    
    AddState(std::move(state));
  }

  {
    ICozmoBehaviorPtr behavior = BC.FindBehaviorByID(BehaviorID::VictorDemoNappingState);
    DEV_ASSERT(behavior != nullptr, "ObservingDemo.NoBehavior.VictorDemoNappingState");

    State state(StateID::Napping, behavior);
    state.AddInterruptingTransition(StateID::WakingUp, CondOffCharger);
    state.AddNonInterruptingTransition(StateID::WakingUp, CondIsHungry);
    
    AddState(std::move(state));
  }

  {
    ICozmoBehaviorPtr behavior = BC.FindBehaviorByID(BehaviorID::VictorDemoNappingWakeUp);
    DEV_ASSERT(behavior != nullptr, "ObservingDemo.NoBehavior.VictorDemoNappingWakeUp");

    State state(StateID::WakingUp, behavior);
    state.AddExitTransition(StateID::ObservingOnCharger, CondTrue);
    
    AddState(std::move(state));
  }

  DEV_ASSERT_MSG( _states.size() == ((size_t)StateID::Count),
                  "BehaviorVictorObservingDemo.MissingStates",
                  "Demo has %zu states defined, but there are %zu in the enum",
                  _states.size(),
                  ((size_t)StateID::Count) );    

  // TODO:(bn) assert that all states are present in the map

  _initComplete = true;
}

void BehaviorVictorObservingDemo::AddState( State&& state )
{
  if( ANKI_VERIFY(!_initComplete, "BehaviorVictorObservingDemo.AddState.CalledOutsideInit", "") &&
      ANKI_VERIFY( _states.find(state._id) == _states.end(),
                   "BehaviorVictorObservingDemo.AddState.StateAlreadyExists", "") ) {    
    _states.emplace(state._id, state);
  }
}

void BehaviorVictorObservingDemo::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  for( const auto& statePair : _states ) {
    if( statePair.second._behavior != nullptr ) {
      delegates.insert(statePair.second._behavior.get());
    }
  }
}


Result BehaviorVictorObservingDemo::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  const bool onCharger = behaviorExternalInterface.GetRobot().IsOnChargerPlatform();

  if( onCharger ) {
    TransitionToState(behaviorExternalInterface, StateID::ObservingOnCharger);
  }
  else {
    TransitionToState(behaviorExternalInterface, StateID::Observing);
  }

  return Result::RESULT_OK;
}

void BehaviorVictorObservingDemo::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  TransitionToState(behaviorExternalInterface, StateID::Count);
}

ICozmoBehavior::Status BehaviorVictorObservingDemo::UpdateInternal_WhileRunning(
  BehaviorExternalInterface& behaviorExternalInterface)
{

  DEV_ASSERT(_currState != StateID::Count, "VictorObservingDemo.Update.InvalidState");
  
  State& state = _states.at(_currState);

  // first check the interrupting conditions
  for( const auto& transitionPair : state._interruptingTransitions ) {
    const auto stateID = transitionPair.first;
    const auto& iConditionPtr = transitionPair.second;

    if( iConditionPtr->Evaluate(behaviorExternalInterface) ) {
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
      
      if( iConditionPtr->Evaluate(behaviorExternalInterface) ) {
        TransitionToState(behaviorExternalInterface, stateID);
        return Status::Running;
      }
    }

    if( ! IsControlDelegated() ) {
      // Now there have been no interrupting or non-interrupting transitions, so check the exit conditions
      for( const auto& transitionPair : state._exitTransitions ) {
        const auto stateID = transitionPair.first;
        const auto& iConditionPtr = transitionPair.second;
      
        if( iConditionPtr->Evaluate(behaviorExternalInterface) ) {
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
  
  if( _currState != StateID::Count ) {
    _states.at(_currState).OnDeactivated();
    const bool allowCallback = false;
    CancelDelegates(allowCallback);
  }
  else {
    DEV_ASSERT( !IsControlDelegated(), "VictorObservingDemo.TransitionToState.WasInCountButHadDelegate" );
  }
          
  // TODO:(bn) channel for demo?
  PRINT_CH_INFO("Unfiltered", "VictorObservingDemo.TransitionToState",
                "Transition from state '%s' -> '%s'",
                _currState  != StateID::Count ? _states.at(_currState )._behavior->GetIDStr().c_str() : "<NONE>",
                targetState != StateID::Count ? _states.at(targetState)._behavior->GetIDStr().c_str() : "<NONE>");

  _currState = targetState;

  if( _currState != StateID::Count ) {
    State& state = _states.at(_currState);

    state.OnActivated();

    if( state._behavior->WantsToBeActivated( behaviorExternalInterface ) ) {
      DelegateIfInControl(behaviorExternalInterface,  state._behavior.get() );
    }

  }
}


BehaviorVictorObservingDemo::State::State(StateID id, ICozmoBehaviorPtr behavior)
  : _id(id)
  , _behavior(behavior)
{
}

void BehaviorVictorObservingDemo::State::AddInterruptingTransition(StateID toState,
                                                                   std::shared_ptr<ICondition> condition)
{
  // TODO:(bn) references / rvalue / avoid copies?
  _interruptingTransitions.emplace_back(toState, condition);
}

void BehaviorVictorObservingDemo::State::AddNonInterruptingTransition(StateID toState,
                                                                      std::shared_ptr<ICondition> condition )
{
  _nonInterruptingTransitions.emplace_back(toState, condition);
}

void BehaviorVictorObservingDemo::State::AddExitTransition(StateID toState, std::shared_ptr<ICondition> condition)
{
  _exitTransitions.emplace_back(toState, condition);
}

void BehaviorVictorObservingDemo::State::OnActivated()
{
  for( const auto& transitionPair : _interruptingTransitions ) {
    const auto& iConditionPtr = transitionPair.second;
    iConditionPtr->EnteredScope();
  }
  for( const auto& transitionPair : _nonInterruptingTransitions ) {
    const auto& iConditionPtr = transitionPair.second;
    iConditionPtr->EnteredScope();
  }
  for( const auto& transitionPair : _exitTransitions ) {
    const auto& iConditionPtr = transitionPair.second;
    iConditionPtr->EnteredScope();
  }

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _lastTimeStarted_s = currTime_s;       
}

void BehaviorVictorObservingDemo::State::OnDeactivated()
{
  for( const auto& transitionPair : _interruptingTransitions ) {
    const auto& iConditionPtr = transitionPair.second;
    iConditionPtr->LeftScope();
  }
  for( const auto& transitionPair : _nonInterruptingTransitions ) {
    const auto& iConditionPtr = transitionPair.second;
    iConditionPtr->LeftScope();
  }
  for( const auto& transitionPair : _exitTransitions ) {
    const auto& iConditionPtr = transitionPair.second;
    iConditionPtr->LeftScope();
  }

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _lastTimeEnded_s = currTime_s;
}

bool BehaviorVictorObservingDemo::StateExitCooldownExpired(StateID state, float timeout) const
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if( _states.at(state)._lastTimeEnded_s < 0.0f ||
      _states.at(state)._lastTimeEnded_s + timeout <= currTime_s ) {
    return true;
  }
  else {
    return false;
  }
}


}
}
