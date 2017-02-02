/**
 * File: iReactionTriggerStrategy.cpp
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
  
  IReactionTriggerStrategy(Robot& robot, const Json::Value& config, const std::string& strategyName);
  virtual ~IReactionTriggerStrategy() {};

  const std::string& GetName() const{ return _strategyName;}
  ReactionTrigger GetReactionTrigger(){ return _triggerID;}
  
  bool IsReactionEnabled() const { return _disableIDs.empty(); }

  // behavior manager checks the return value of this function every tick
  // to see if the reactionary behavior has requested a computational switch
  // override to trigger a reactionary behavior based on something other than a message
  virtual bool ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior) = 0;
  
  // if true, the previously running behavior will be resumed (if possible) after the behavior triggered by
  // this trigger is complete. Otherwise, a new behavior will be selected by the chooser.
  virtual bool ShouldResumeLastBehavior() const = 0;

  // if this returns false, this trigger cannot run while a
  // triggered behavior is running
  virtual bool CanTriggerWhileTriggeredBehaviorRunning() const { return true; }
  
  // allows reaction triggers to check should computationally switch
  // even if they are currently running
  virtual bool CanInterruptSelf() const { return false; }
  
  // Derived classes can override this function if they want to add listeners
  // to the behavior they will trigger
  virtual void BehaviorThatStartegyWillTrigger(IBehavior* behavior) {};
  
protected:
  using GameToEngineEvent = AnkiEvent<ExternalInterface::MessageGameToEngine>;
  using EngineToGameEvent = AnkiEvent<ExternalInterface::MessageEngineToGame>;
  using EngineToGameTag   = ExternalInterface::MessageEngineToGameTag;
  using GameToEngineTag   = ExternalInterface::MessageGameToEngineTag;
  
  // Derived classes should use these methods to subscribe to any tags they
  // are interested in handling.
  void SubscribeToTags(std::set<GameToEngineTag>&& tags);
  void SubscribeToTags(std::set<EngineToGameTag>&& tags);
  
  void AlwaysHandle(const GameToEngineEvent& event, const Robot& robot);
  void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot);
  virtual void AlwaysHandleInternal(const GameToEngineEvent& event, const Robot& robot) { }
  virtual void AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot) { }

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
  
  
private:
  Robot& _robot;
  const std::string _strategyName;
  ReactionTrigger _triggerID = ReactionTrigger::NoneTrigger;
  std::multiset<std::string> _disableIDs;
  std::vector<::Signal::SmartHandle> _eventHandles;
  
  template<class EventType>
  void HandleEvent(const EventType& event);
  
  // Handle tracking enable/disable requests
  // Returns true if ids are updated (requsterID doesn't exist/exsits for enabling/disabling)
  // false otherwise
  virtual bool UpdateDisableIDs(const std::string& requesterID, bool enable);

};

template<class EventType>
void IReactionTriggerStrategy::HandleEvent(const EventType& event)
{
  AlwaysHandle(event, _robot);
}

} // namespace Cozmo
} // namespace Anki

#endif
