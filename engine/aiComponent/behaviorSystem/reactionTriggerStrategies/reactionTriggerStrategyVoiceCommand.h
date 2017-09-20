/**
* File: reactionTriggerStrategyVoiceCommand.h
*
* Author: Lee Crippen
* Created: 02/16/17
*
* Description: Reaction trigger strategy for hearing a voice command.
*
* Copyright: Anki, Inc. 2017
*
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyVoiceCommand_H__
#define __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyVoiceCommand_H__

#include "engine/aiComponent/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"
#include "engine/aiComponent/behaviorSystem/behaviorListenerInterfaces/iSubtaskListener.h"
#include "anki/vision/basestation/faceIdTypes.h"

#include <map>
#include <set>

namespace Anki {
namespace Cozmo {

  
class ReactionTriggerStrategyVoiceCommand : public IReactionTriggerStrategy, public ISubtaskListener {
public:
  ReactionTriggerStrategyVoiceCommand(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config);

  virtual bool ShouldResumeLastBehavior() const override { return true; }
  virtual bool CanInterruptOtherTriggeredBehavior() const override { return true; }
  
  virtual void AnimationComplete(BehaviorExternalInterface& behaviorExternalInterface) override;
  
protected:
  virtual void EnabledStateChanged(BehaviorExternalInterface& behaviorExternalInterface, bool enabled) override {_shouldTrigger = false;}
  virtual bool ShouldTriggerBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr behavior) override;
  virtual void SetupForceTriggerBehavior(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr behavior) override;
  virtual void BehaviorThatStrategyWillTriggerInternal(IBehaviorPtr behavior) override;

private:
  bool                                  _shouldTrigger = false;
  std::map<Vision::FaceID_t, float>     _lookedAtTimesMap;
  
  Vision::FaceID_t GetDesiredFace(BehaviorExternalInterface& behaviorExternalInterface) const;
  
  // Json params
  bool _isWakeUpReaction = false;
};


} // namespace Cozmo
} // namespace Anki

#endif //
