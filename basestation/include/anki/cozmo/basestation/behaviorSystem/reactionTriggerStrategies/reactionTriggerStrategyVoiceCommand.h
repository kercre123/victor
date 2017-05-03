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

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"
#include "anki/vision/basestation/faceIdTypes.h"

#include <map>
#include <set>

namespace Anki {
namespace Cozmo {

  
#if (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)
class ReactionTriggerStrategyVoiceCommand : public IReactionTriggerStrategy {
public:
  ReactionTriggerStrategyVoiceCommand(Robot& robot, const Json::Value& config);

  virtual bool ShouldResumeLastBehavior() const override { return true; }
  virtual bool CanInterruptOtherTriggeredBehavior() const override { return true; }
  
protected:
  virtual void EnabledStateChanged(bool enabled) override {_shouldTrigger = false;}
  virtual bool ShouldTriggerBehaviorInternal(const Robot& robot, const IBehavior* behavior) override;
  virtual void SetupForceTriggerBehavior(const Robot& robot, const IBehavior* behavior) override;

private:
  bool                                  _shouldTrigger = false;
  std::map<Vision::FaceID_t, float>     _lookedAtTimesMap;
  
  Vision::FaceID_t GetDesiredFace(const Robot& robot) const;
};
#endif // (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)


} // namespace Cozmo
} // namespace Anki

#endif //
