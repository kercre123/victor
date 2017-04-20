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

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorListenerInterfaces/iReactToPetListener.h"

#include "anki/vision/basestation/faceIdTypes.h"

namespace Anki {
namespace Cozmo {

class ReactionTriggerStrategyPetInitialDetection : public IReactionTriggerStrategy, public IReactToPetListener {
public:
  ReactionTriggerStrategyPetInitialDetection(Robot& robot, const Json::Value& config);

  virtual bool ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior) override;
  virtual bool ShouldResumeLastBehavior() const override { return true;}
  virtual bool CanInterruptOtherTriggeredBehavior() const override { return true; }
  
  virtual void EnabledStateChanged(bool enabled) override
                 {UpdateReactedTo(_robot);}
  
  // Implementation of IReactToPetListener
  virtual void BehaviorDidReact(const std::set<Vision::FaceID_t> & targets) override;

  
protected:
  virtual void BehaviorThatStrategyWillTrigger(IBehavior* behavior) override;
  
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
