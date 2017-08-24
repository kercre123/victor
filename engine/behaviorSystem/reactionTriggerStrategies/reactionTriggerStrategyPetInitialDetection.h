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

#include "engine/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"
#include "engine/behaviorSystem/behaviorListenerInterfaces/iReactToPetListener.h"

#include "anki/vision/basestation/faceIdTypes.h"

namespace Anki {
namespace Cozmo {

class ReactionTriggerStrategyPetInitialDetection : public IReactionTriggerStrategy, public IReactToPetListener {
public:
  ReactionTriggerStrategyPetInitialDetection(Robot& robot, const Json::Value& config);

  virtual bool ShouldResumeLastBehavior() const override { return true;}
  virtual bool CanInterruptOtherTriggeredBehavior() const override { return true; }
  
  virtual void EnabledStateChanged(const Robot& robot, bool enabled) override
                 {UpdateReactedTo(_robot);}
  
  // Implementation of IReactToPetListener
  virtual void BehaviorDidReact(const std::set<Vision::FaceID_t> & targets) override;

  
protected:
  virtual void BehaviorThatStrategyWillTriggerInternal(IBehaviorPtr behavior) override;

  virtual bool ShouldTriggerBehaviorInternal(const Robot& robot, const IBehaviorPtr behavior) override;
  virtual void SetupForceTriggerBehavior(const Robot& robot, const IBehaviorPtr behavior) override;
  
private:
  // Illegal time value to represent "never"
  static constexpr float NEVER = -1.0f;

  // Reference to associated robot
  Robot& _robot;
  
  // Everything we have already reacted to
  std::set<Vision::FaceID_t> _reactedTo;
  
  // Last time we reacted to any target
  float _lastReactionTime_s = NEVER;
  
  // Internal helpers
  bool RecentlyReacted() const;
  void InitReactedTo(const Robot& robot);
  void UpdateReactedTo(const Robot& robot);
  
};


} // namespace Cozmo
} // namespace Anki

#endif //
