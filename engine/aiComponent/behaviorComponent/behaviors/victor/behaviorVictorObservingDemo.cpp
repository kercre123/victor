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

#include "clad/types/behaviorSystem/behaviorTypes.h"
#include "clad/types/needsSystemTypes.h"
#include "coretech/common/include/anki/common/basestation/utils/timer.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorListenerInterfaces/iFeedingListener.h"
#include "engine/aiComponent/behaviorComponent/behaviors/feeding/behaviorFeedingEat.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/needsSystem/needsState.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

namespace {

static constexpr const float kFeedingTimeout_s = 30.0f;

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

        if( currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Warning) ||
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
    [this](BehaviorExternalInterface& behaviorExternalInterface) {
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
  
  ////////////////////////////////////////////////////////////////////////////////
  // Define states
  ////////////////////////////////////////////////////////////////////////////////

  const auto& BC = behaviorExternalInterface.GetBehaviorContainer();

  {
    ICozmoBehaviorPtr behavior = BC.FindBehaviorByID(BehaviorID::VictorDemoObservingOnChargerState);
    DEV_ASSERT(behavior != nullptr, "ObservingDemo.NoBehavior.VictorDemoObservingOnChargerState");

    State observingOnCharger(StateID::ObservingOnCharger, behavior);
    observingOnCharger.AddNonInterruptingTransition(StateID::Observing, CondIsHungry);
    observingOnCharger.AddInterruptingTransition(StateID::Observing, CondOffCharger);
    observingOnCharger.AddInterruptingTransition(StateID::Feeding, CondCanEat);
    AddState(std::move(observingOnCharger));
  }

  {
    ICozmoBehaviorPtr behavior = BC.FindBehaviorByID(BehaviorID::VictorDemoObservingState);
    DEV_ASSERT(behavior != nullptr, "ObservingDemo.NoBehavior.VictorDemoObservingState");

    State observing(StateID::Observing, behavior);
    // observing.AddInterruptingTransition(StateID::ObservingOnCharger, CondOnCharger);
    //  // TODO:(bn) need to differentiate between "placed on charger" and "on charger" coming from the other state
    // OR: add a transition state that takes us off the charger
    observing.AddInterruptingTransition(StateID::Feeding, CondCanEat);
    AddState(std::move(observing));
  }

  {
    ICozmoBehaviorPtr behavior = BC.FindBehaviorByID(BehaviorID::VictorDemoFeedingState);
    DEV_ASSERT(behavior != nullptr, "ObservingDemo.NoBehavior.VictorDemoFeedingState");

    State feeding(StateID::Feeding, behavior);
    feeding.AddNonInterruptingTransition(StateID::Observing, std::make_shared<TimeoutCondition>(kFeedingTimeout_s));

    if( _feedingCompleteCondition ) {
      feeding.AddNonInterruptingTransition(StateID::Observing, _feedingCompleteCondition);
    }        
    
    AddState(std::move(feeding));
  }

  // TODO:(bn) assert that all states are present in the map

  _initComplete = true;
}

void BehaviorVictorObservingDemo::AddState( State&& state )
{
  if( ANKI_VERIFY(!_initComplete, "BehaviorVictorObservingDemo.AddState.CalledOutsideInit", "") ) {
    _states.emplace(state._id, state);
  }
}

void BehaviorVictorObservingDemo::GetAllDelegatesInternal(std::set<IBehavior*>& delegates) const
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
    const auto& IConditionPtr = transitionPair.second;

    if( IConditionPtr->Evaluate(behaviorExternalInterface) ) {
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
      const auto& IConditionPtr = transitionPair.second;
      
      if( IConditionPtr->Evaluate(behaviorExternalInterface) ) {
        TransitionToState(behaviorExternalInterface, stateID);
        return Status::Running;
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

void BehaviorVictorObservingDemo::State::OnActivated()
{
  for( const auto& transitionPair : _interruptingTransitions ) {
    const auto& IConditionPtr = transitionPair.second;
    IConditionPtr->EnteredScope();
  }
  for( const auto& transitionPair : _nonInterruptingTransitions ) {
    const auto& IConditionPtr = transitionPair.second;
    IConditionPtr->EnteredScope();
  }
}

void BehaviorVictorObservingDemo::State::OnDeactivated()
{
  for( const auto& transitionPair : _interruptingTransitions ) {
    const auto& IConditionPtr = transitionPair.second;
    IConditionPtr->LeftScope();
  }
  for( const auto& transitionPair : _nonInterruptingTransitions ) {
    const auto& IConditionPtr = transitionPair.second;
    IConditionPtr->LeftScope();
  }
}

}
}
