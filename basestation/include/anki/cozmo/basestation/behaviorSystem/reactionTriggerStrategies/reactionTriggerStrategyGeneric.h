/**
* File: reactionTriggerStrategyGeneric.h
*
* Author: Lee Crippen
* Created: 02/15/2017
*
* Description: Generic Reaction Trigger strategy for responding to many configurations of reaction triggers.
*
* Copyright: Anki, Inc. 2017
*
*
**/
#ifndef __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyGeneric_H__
#define __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyGeneric_H__

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"

#include <set>
#include <string>

namespace Anki {
namespace Cozmo {

class ReactionTriggerStrategyGeneric : public IReactionTriggerStrategy{
public:
  // Static function for creating instances that makes use of json configuration during construction
  static ReactionTriggerStrategyGeneric* CreateReactionTriggerStrategyGeneric(Robot& robot,
                                                                              const Json::Value& config);
  
  // Functionality being overridden from base class
  virtual bool ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior) override;
  virtual bool ShouldResumeLastBehavior() const override { return _shouldResumeLast;}
  virtual bool CanInterruptOtherTriggeredBehavior() const override { return _canInterruptOtherTriggeredBehavior; }
  
  // Callback for the custom logic to determine whether to trigger the behavior
  using ShouldTriggerCallbackType = std::function<bool(const Robot& robot, const IBehavior* behavior)>;
  
  // Callback for the custom logic to determine whether to computationally switch based on this event
  using EventHandleCallbackType = std::function<bool(const EngineToGameEvent& event, const Robot& robot)>;
  
  // Setters for the above callbacks
  void SetShouldTriggerCallback(ShouldTriggerCallbackType callback) { _shouldTriggerCallback = callback; }
  void ConfigureRelevantEvents(std::set<EngineToGameTag> relevantEvents, EventHandleCallbackType callback = EventHandleCallbackType{});
  
protected:
  virtual void AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot) override;
  virtual void EnabledStateChanged(bool enabled) override {_shouldTrigger = false;}
  
private:
  bool                          _shouldTrigger = false;
  std::string                   _strategyName = "Trigger Strategy Generic";
  bool                          _shouldResumeLast = true;
  bool                          _canInterruptOtherTriggeredBehavior; // Initialized in constructor by call to base class getter
  bool                          _needsRobotPreReq = false;
  std::set<EngineToGameTag>     _relevantEvents; // The list of events this trigger is subscribed to, kept for sanity checking later
  ShouldTriggerCallbackType     _shouldTriggerCallback;
  EventHandleCallbackType       _eventHandleCallback;
  
  // Private constructor because we want to create instances using the static creation function above
  ReactionTriggerStrategyGeneric(Robot& robot,
                                 const Json::Value& config,
                                 std::string strategyName);
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyGeneric_H__
