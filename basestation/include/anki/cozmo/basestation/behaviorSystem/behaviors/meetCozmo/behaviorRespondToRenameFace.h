/**
 * File: behaviorRespondToRenameFace.h
 *
 * Author: Andrew Stein
 * Created: 12/13/2016
 *
 * Description: Behavior for responding to a face being renamed
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorRespondToRenameFace_H__
#define __Cozmo_Basestation_Behaviors_BehaviorRespondToRenameFace_H__

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorRespondToRenameFace : public IBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorRespondToRenameFace(Robot& robot, const Json::Value& config);
  
public:

  virtual bool IsRunnableInternal(const BehaviorPreReqNone& preReqData ) const override;
  virtual bool CarryingObjectHandledInternally() const override { return false; }
  
protected:
  
  virtual Result InitInternal(Robot& robot)   override;
  virtual Status UpdateInternal(Robot& robot) override;
  virtual void   HandleWhileNotRunning(const GameToEngineEvent& event, const Robot& robot) override;
  
private:
  
  std::string      _name;
  Vision::FaceID_t _faceID;
  AnimationTrigger _animTrigger; // set by Json config
  
}; // class BehaviorReactToRenameFace
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorRespondToRenameFace_H__
