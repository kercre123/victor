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

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
#include "anki/vision/basestation/faceIdTypes.h"

namespace Anki {
namespace Cozmo {

class Robot;
  
class BehaviorReactToVoiceCommand : public IBehavior
{
private:
  using super = IBehavior;
  
  friend class BehaviorFactory;
  BehaviorReactToVoiceCommand(Robot& robot, const Json::Value& config);
  
public:
  virtual bool IsRunnableInternal(const BehaviorPreReqAcknowledgeFace& preReqData ) const override;
  virtual bool CarryingObjectHandledInternally() const override {return true;}
  
protected:
  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;
  
private:
  mutable Vision::FaceID_t _desiredFace = Vision::UnknownFaceID;
  
}; // class BehaviorReactToVoiceCommand

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToVoiceCommand_H__
