/**
 * File: iReactionTriggerStrategy.h
 *
 * Author: Kevin M. Karol
 * Created: 12/08/16
 *
 * Description: Interface for defining when a reaction trigger should fire its
 * mapped behavior.  Strategy to behavior map is defined in behaviorManager
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_iReactionTriggerStrategy_H__
#define __Cozmo_Basestation_BehaviorSystem_iReactionTriggerStrategy_H__

#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqNone.h"

#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "json/json-forwards.h"
#include "util/random/randomGenerator.h"
#include "util/signals/simpleSignal_fwd.h"

#include <set>

namespace Anki {
namespace Cozmo {

class Robot;
class IBehavior;

namespace ReactionTriggerConst{
static const BehaviorPreReqNone kNoPreReqs;
}
  
class IReactionTriggerStrategy{
public:
  // Allows the factory to set trigger type
  friend class ReactionTriggerStrategyFactory;
  // Allows the behaviorManager to notify of enabled state changes
  friend class BehaviorManager;
  
  IReactionTriggerStrategy(Robot& robot, const Json::Value& config, const std::string& strategyName);
  virtual ~IReactionTriggerStrategy() {};

  const std::string& GetName() const { return _strategyName;}
  ReactionTrigger GetReactionTrigger() const { return _triggerID;}


  // behavior manager checks the return value of this function every tick
  // to see if the reactionary behavior has requested a computational switch
  // override to trigger a reactionary behavior based on something other than a message
  bool ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior);
  
  // if true, the previously running behavior will be resumed (if possible) after the behavior triggered by
  // this trigger is complete. Otherwise, a new behavior will be selected by the chooser.
  virtual bool ShouldResumeLastBehavior() const = 0;

  // If this returns true, then this reaction trigger can interrupt any behavior
  //  that was caused by any _other_ reaction trigger
  virtual bool CanInterruptOtherTriggeredBehavior() const { return true; }
  
  // allows reaction triggers to check should computationally switch
  // even if they are currently running
  virtual bool CanInterruptSelf() const { return false; }
  
  // Derived classes can override this function if they want to add listeners
  // to the behavior they will trigger
  void BehaviorThatStrategyWillTrigger(IBehavior* behavior);

  // A random number generator all subclasses can share
  Util::RandomGenerator& GetRNG() const;
  
protected:
  using GameToEngineEvent = AnkiEvent<ExternalInterface::MessageGameToEngine>;
  using EngineToGameEvent = AnkiEvent<ExternalInterface::MessageEngineToGame>;
  using EngineToGameTag   = ExternalInterface::MessageEngineToGameTag;
  using GameToEngineTag   = ExternalInterface::MessageGameToEngineTag;
  
  // Derived classes should use these methods to subscribe to any tags they
  // are interested in handling.
  void SubscribeToTags(std::set<GameToEngineTag>&& tags);
  void SubscribeToTags(std::set<EngineToGameTag>&& tags);
  
  virtual void AlwaysHandleInternal(const GameToEngineEvent& event, const Robot& robot) {}
  virtual void AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot) {}

  // Override if you want to respond to being enabled/disabled
  // by RequestEnableReactionaryBehavior message
  virtual void EnabledStateChanged(bool enabled) {};
  
  // used by the ReactionTriggerStrategy Factory to set the reaction trigger type
  void SetReactionTrigger(ReactionTrigger trigger)
  {
    if(_triggerID == ReactionTrigger::NoneTrigger){
      _triggerID = trigger;
    }
  }

  // Debug user is about to force a behavior
  // each behavior needs to be able to handle "gracefully" a transition into starting the behavior
  virtual void SetupForceTriggerBehavior(const Robot& robot, const IBehavior* behavior) = 0;

  // behavior manager checks the return value of this function every tick
  // to see if the reactionary behavior has requested a computational switch
  // override to trigger a reactionary behavior based on something other than a message
  virtual bool ShouldTriggerBehaviorInternal(const Robot& robot, const IBehavior* behavior) = 0;
 
  // Derived classes can override this function if they want to add listeners
  // to the behavior they will trigger
  virtual void BehaviorThatStrategyWillTriggerInternal(IBehavior* behavior){}

private:
  Robot& _robot;
  const std::string _strategyName;
  ReactionTrigger _triggerID = ReactionTrigger::NoneTrigger;
  std::vector<::Signal::SmartHandle> _eventHandles;
  std::string _DebugBehaviorName;

  void AlwaysHandle(const GameToEngineEvent& event, const Robot& robot);
  void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot);
  
  template<class EventType>
  void HandleEvent(const EventType& event);
  
protected:
  bool _userForcingTrigger;
};

template<class EventType>
void IReactionTriggerStrategy::HandleEvent(const EventType& event)
{
  AlwaysHandle(event, _robot);
}

} // namespace Cozmo
} // namespace Anki

#endif
