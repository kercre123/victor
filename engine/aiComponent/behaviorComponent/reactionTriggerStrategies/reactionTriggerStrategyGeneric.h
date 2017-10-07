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

#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/iReactionTriggerStrategy.h"
#include "engine/aiComponent/behaviorComponent/wantsToRunStrategies/strategyGeneric.h"

#include <set>
#include <string>

namespace Anki {
namespace Cozmo {

class ReactionTriggerStrategyGeneric : public IReactionTriggerStrategy {
public:
  // Static function for creating instances that makes use of json configuration during construction
  static ReactionTriggerStrategyGeneric* CreateReactionTriggerStrategyGeneric(BehaviorExternalInterface& behaviorExternalInterface,
                                                                              IExternalInterface* robotExternalInterface,
                                                                              const Json::Value& config);
  
  // Functionality being overridden from base class
  virtual bool ShouldResumeLastBehavior() const override { return _shouldResumeLast;}
  virtual bool CanInterruptOtherTriggeredBehavior() const override { return _canInterruptOtherTriggeredBehavior; }
  
  void SetShouldTriggerCallback(StrategyGeneric::ShouldTriggerCallbackType callback);
  
  void ConfigureRelevantEvents(std::set<EngineToGameTag> relevantEvents,
                               StrategyGeneric::E2GHandleCallbackType callback = StrategyGeneric::E2GHandleCallbackType{});
  
  void ConfigureRelevantEvents(std::set<GameToEngineTag> relevantEvents,
                               StrategyGeneric::G2EHandleCallbackType callback = StrategyGeneric::G2EHandleCallbackType{});
  
protected:
  virtual void EnabledStateChanged(BehaviorExternalInterface& behaviorExternalInterface, bool enabled) override {_shouldTrigger = false;}

  virtual bool ShouldTriggerBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr behavior) override;
  virtual void SetupForceTriggerBehavior(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr behavior) override;
  
private:
  
  StrategyGeneric* GetGenericWantsToRunStrategy() const;

  bool                          _shouldTrigger = false;
  std::string                   _strategyName = "Trigger Strategy Generic";
  bool                          _shouldResumeLast = true;
  bool                          _canInterruptOtherTriggeredBehavior; // Initialized in constructor by call to base class getter
  bool                          _needsRobotPreReq = false;
  
  // Private constructor because we want to create instances using the static creation function above
  ReactionTriggerStrategyGeneric(BehaviorExternalInterface& behaviorExternalInterface,
                                 IExternalInterface* robotExternalInterface,
                                 const Json::Value& config,
                                 std::string strategyName);
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyGeneric_H__
