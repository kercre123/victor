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

#include "engine/aiComponent/behaviorSystem/behaviors/iBehavior.h"
#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorRespondToRenameFace : public IBehavior
{
private:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorRespondToRenameFace(const Json::Value& config);
  
public:

  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override { return false; }
  
protected:
  
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)   override;
  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void   HandleWhileNotRunning(const GameToEngineEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  
private:
  
  std::string      _name;
  Vision::FaceID_t _faceID;
  AnimationTrigger _animTrigger; // set by Json config
  
}; // class BehaviorReactToRenameFace
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorRespondToRenameFace_H__
