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

#include "engine/aiComponent/behaviorSystem/behaviors/iBehavior.h"
#include "anki/vision/basestation/faceIdTypes.h"

namespace Anki {
namespace Cozmo {

class Robot;
  
class BehaviorReactToVoiceCommand : public IBehavior
{
private:
  using super = IBehavior;
  
  friend class BehaviorContainer;
  BehaviorReactToVoiceCommand(const Json::Value& config);
  
public:
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override {return true;}
  void SetDesiredFace(const Vision::FaceID_t& desiredFace){_desiredFace = desiredFace;}
  
  // Empty override of AddListener because the strategy that controls this behavior is a listener
  // The strategy controls multiple different behaviors and listeners are necessary for the other behaviors
  // since they are generic PlayAnim behaviors (reactToVoiceCommand_Wakeup)
  virtual void AddListener(ISubtaskListener* listener) override {};
  
protected:
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void   OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  
private:
  mutable Vision::FaceID_t _desiredFace = Vision::UnknownFaceID;
  
}; // class BehaviorReactToVoiceCommand

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToVoiceCommand_H__
