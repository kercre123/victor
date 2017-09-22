/**
 * File: reactionTriggerStrategyPet.h
 *
 * Author: Kevin M. Karol
 * Created: 12/08/16
 *
 * Description: Reaction Trigger strategy for responding to seeing a pet
 *
 * Copyright: Anki, Inc. 2016
 *
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyPet_H__
#define __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyPet_H__

#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/iReactionTriggerStrategy.h"
#include "engine/aiComponent/behaviorComponent/behaviorListenerInterfaces/iReactToPetListener.h"

#include "anki/vision/basestation/faceIdTypes.h"

namespace Anki {
namespace Cozmo {

class ReactionTriggerStrategyPetInitialDetection : public IReactionTriggerStrategy, public IReactToPetListener {
public:
  ReactionTriggerStrategyPetInitialDetection(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config);

  virtual bool ShouldResumeLastBehavior() const override { return true;}
  virtual bool CanInterruptOtherTriggeredBehavior() const override { return true; }
  
  virtual void EnabledStateChanged(BehaviorExternalInterface& behaviorExternalInterface, bool enabled) override
                 {UpdateReactedTo(behaviorExternalInterface);}
  
  // Implementation of IReactToPetListener
  virtual void BehaviorDidReact(const std::set<Vision::FaceID_t> & targets) override;

  
protected:
  virtual void BehaviorThatStrategyWillTriggerInternal(IBehaviorPtr behavior) override;

  virtual bool ShouldTriggerBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr behavior) override;
  virtual void SetupForceTriggerBehavior(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr behavior) override;
  
private:
  BehaviorExternalInterface& _behaviorExternalInterface;
  // Illegal time value to represent "never"
  static constexpr float NEVER = -1.0f;
  
  // Everything we have already reacted to
  std::set<Vision::FaceID_t> _reactedTo;
  
  // Last time we reacted to any target
  float _lastReactionTime_s = NEVER;
  
  // Internal helpers
  bool RecentlyReacted() const;
  void InitReactedTo(BehaviorExternalInterface& behaviorExternalInterface);
  void UpdateReactedTo(BehaviorExternalInterface& behaviorExternalInterface);
  
};


} // namespace Cozmo
} // namespace Anki

#endif //
