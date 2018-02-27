/**
* File: behaviorReactToVoiceCommand.h
*
* Author: Lee Crippen
* Created: 2/16/2017
*
* Description: Simple behavior to immediately respond to the voice command keyphrase, while waiting for further commands.
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToVoiceCommand_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToVoiceCommand_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "coretech/vision/engine/faceIdTypes.h"

#include "engine/smartFaceId.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorReactToVoiceCommand : public ICozmoBehavior
{
private:
  using super = ICozmoBehavior;
  
  friend class BehaviorFactory;
  BehaviorReactToVoiceCommand(const Json::Value& config);
  
public:
  virtual bool WantsToBeActivatedBehavior() const override;
  void SetDesiredFace(const SmartFaceID& desiredFace){_desiredFace = desiredFace;}
  
  // Empty override of AddListener because the strategy that controls this behavior is a listener
  // The strategy controls multiple different behaviors and listeners are necessary for the other behaviors
  // since they are generic PlayAnim behaviors (reactToVoiceCommand_Wakeup)
  virtual void AddListener(ISubtaskListener* listener) override {};
  
protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
  }

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;
  
private:
  mutable SmartFaceID _desiredFace;
  
  // json params and their defaults
  AnimationTrigger _animUnmatched = AnimationTrigger::Count;
  bool _exitOnIntents = true;
  
}; // class BehaviorReactToVoiceCommand

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToVoiceCommand_H__
