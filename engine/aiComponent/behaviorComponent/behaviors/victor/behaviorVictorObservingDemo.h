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
#include "clad/types/visionModes.h"
#include "engine/components/bodyLightComponent.h"

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

  virtual ~BehaviorVictorObservingDemo();
  
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
    DriveOffChargerIntoSocializing,
    DriveOffChargerIntoFeeding,
    Observing,
    Feeding,
    Socializing,
    Napping,
    WakingUp,
    Playing,
    ReturningToCharger,
    FailedToFindCharger,

    Count
  };
  
  class State;

  void AddState( State&& state );

  void TransitionToState(BehaviorExternalInterface& behaviorExternalInterface, const StateID targetState);

  bool StateExitCooldownExpired(StateID state, float timeout) const;

  using StateMap = std::map< StateID, State >;
  std::unique_ptr< StateMap > _states;

  std::shared_ptr<FeedingListenerCondition> _feedingCompleteCondition;

  // hack to turn off all modes when this behavior starts and then back on when it ends
  std::vector< VisionMode > _visionModesToReEnable;

  StateID _currState = StateID::Count;

  bool _initComplete = false;

  BackpackLights _currDebugLights;
  bool _debugLightsDirty = false;
  bool _useDebugLights = false;
  float _lastHearbeatLightTime = -1.0f;
};

}
}

#endif
