/**
 * File: behaviorVictorObservingDemo.h
 *
 * Author: Brad Neuman
 * Created: 2017-10-16
 *
 * Description: Root behavior to handle the state machine for the victor observing demo
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_Victor_BehaviorVictorObservingDemo_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_Victor_BehaviorVictorObservingDemo_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "anki/common/basestation/objectIDs.h"

namespace Anki {
namespace Cozmo {

class BehaviorFeedingEat;

// TODO:(bn) combine this with WantsToRunStrategy
class ICondition {
public:
  ICondition() {}
  virtual ~ICondition() {}

  virtual void EnteredScope() { }
  virtual void LeftScope() { }

  virtual bool Evaluate(BehaviorExternalInterface& behaviorExternalInterface) = 0;
};

class LambdaCondition : public ICondition {
public:
  using TransitionFunction = std::function<bool(BehaviorExternalInterface& behaviorExternalInterface)>;
  LambdaCondition(TransitionFunction func) :_func(func) {}

  virtual bool Evaluate(BehaviorExternalInterface& behaviorExternalInterface) override {
    return _func(behaviorExternalInterface);
  }

private:
  TransitionFunction _func;
};

class FeedingListenerCondition;

class BehaviorVictorObservingDemo : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;  
  BehaviorVictorObservingDemo(const Json::Value& config);

public:

  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override {
    return true; }
  
  virtual bool CarryingObjectHandledInternally() const override {return false;}
  virtual bool ShouldRunWhileOnCharger() const override { return true; }
  virtual bool ShouldRunWhileOffTreads() const override { return true;}

protected:

  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;

  virtual void InitBehavior(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void   OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;

private:

  enum class StateID {
    
    ObservingOnCharger = 0,
    ObservingOnChargerRecentlyPlaced,
    DriveOffChargerIntoObserving,
    DriveOffChargerIntoPlay,
    Observing,
    Feeding,
    Socializing,
    Napping,
    WakingUp,
    Playing,

    Count
  };
  
  class State {
  public:
    State(StateID id, ICozmoBehaviorPtr behavior);

    void AddInterruptingTransition(StateID toState, std::shared_ptr<ICondition> condition );
    void AddNonInterruptingTransition(StateID toState, std::shared_ptr<ICondition> condition );
    void AddExitTransition(StateID toState, std::shared_ptr<ICondition> condition);

    void OnActivated();
    void OnDeactivated();
    // TODO:(bn) add asserts for these
    
    StateID _id;
    ICozmoBehaviorPtr _behavior;

    float _lastTimeStarted_s = -1.0f;
    float _lastTimeEnded_s = -1.0f;

    // Transitions are evaluated in order, and if the function returns true, we will transition to the given
    // state id.     
    using Transitions = std::vector< std::pair< StateID, std::shared_ptr<ICondition> > >;

    // transitions that can happen while the state is active (and in the middle of doing something)
    Transitions _interruptingTransitions;

    // transitions that can happen when the currently delegated-to behavior thinks it's a decent time for a
    // gentle interruption (or if there is no currently delegated behavior)
    Transitions _nonInterruptingTransitions;

    // exit transitions only run if the currently-delegated-to behavior stop itself. Note that these are
    // checked _after_ all of the other transitions
    Transitions _exitTransitions;
  };

  void AddState( State&& state );

  void TransitionToState(BehaviorExternalInterface& behaviorExternalInterface, const StateID targetState);

  bool StateExitCooldownExpired(StateID state, float timeout) const;
  
  std::map< StateID, State > _states;

  std::shared_ptr<FeedingListenerCondition> _feedingCompleteCondition;

  StateID _currState = StateID::Count;

  bool _initComplete = false;  
};

}
}

#endif
