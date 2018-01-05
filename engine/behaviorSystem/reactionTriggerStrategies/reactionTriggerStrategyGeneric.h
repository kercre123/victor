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

#include "anki/common/types.h"

#include "engine/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"
#include "engine/behaviorSystem/wantsToRunStrategies/strategyGeneric.h"

#include <set>
#include <string>

namespace Anki {
namespace Cozmo {

class ReactionTriggerStrategyGeneric : public IReactionTriggerStrategy {
public:
  // Static function for creating instances that makes use of json configuration during construction
  static ReactionTriggerStrategyGeneric* CreateReactionTriggerStrategyGeneric(Robot& robot,
                                                                              const Json::Value& config);
  
  // Functionality being overridden from base class
  virtual bool ShouldResumeLastBehavior() const override { return _shouldResumeLast;}
  virtual bool CanInterruptOtherTriggeredBehavior() const override { return _canInterruptOtherTriggeredBehavior; }
  
  void SetShouldTriggerCallback(StrategyGeneric::ShouldTriggerCallbackType callback);
  
  // When strategy generic receives a message, it checks the appropriate callback, and if that
  // callback wants to run the strategy stores the value until the next time WantsToRun is called
  // This can result in messages triggering reactions after an arbitrarily long delay if WantsToRun is not regularly ticked
  // This function causes the ReactionTriggerStrategyGeneric to track the same messages as StrategyGeneric, but store
  // the timestamp at which the messages were received. Then, when ShouldTriggerBehavior is called, the reaction
  // trigger will overrule the strategyGeneric if the message was received longer ago than the timeout and not run the reaction
  // in response to a message after a long delay.
  void ConfigureRelevantEventsWithTimeout(const std::set<EngineToGameTag>& relevantEvents,
                                          StrategyGeneric::E2GHandleCallbackType callback,
                                          int timeout_ms);
  
  void ConfigureRelevantEvents(const std::set<EngineToGameTag>& relevantEvents,
                               StrategyGeneric::E2GHandleCallbackType callback = StrategyGeneric::E2GHandleCallbackType{});
  
  void ConfigureRelevantEvents(const std::set<GameToEngineTag>& relevantEvents,
                               StrategyGeneric::G2EHandleCallbackType callback = StrategyGeneric::G2EHandleCallbackType{});
  
protected:
  virtual void EnabledStateChanged(const Robot& robot, bool enabled) override {_shouldTrigger = false;}

  virtual bool ShouldTriggerBehaviorInternal(const Robot& robot, const IBehaviorPtr behavior) override;
  virtual void SetupForceTriggerBehavior(const Robot& robot, const IBehaviorPtr behavior) override;
  
  virtual void AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot) override;
  
private:
  
  StrategyGeneric* GetGenericWantsToRunStrategy() const;

  bool                          _shouldTrigger = false;
  std::string                   _strategyName = "Trigger Strategy Generic";
  bool                          _shouldResumeLast = true;
  bool                          _canInterruptOtherTriggeredBehavior; // Initialized in constructor by call to base class getter
  bool                          _needsRobotPreReq = false;
  
  // variables for tracking relevant events with timeout
  int _timeout_ms = -1;
  std::set<EngineToGameTag> _timeoutRelatedEventSet;
  std::map<EngineToGameTag, TimeStamp_t> _timedEventToMostRecentTimeMap;
  
  // Private constructor because we want to create instances using the static creation function above
  ReactionTriggerStrategyGeneric(Robot& robot,
                                 const Json::Value& config,
                                 std::string strategyName);
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyGeneric_H__
