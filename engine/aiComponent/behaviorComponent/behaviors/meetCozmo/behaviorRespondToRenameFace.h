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

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Vector {
  
class BehaviorRespondToRenameFace : public ICozmoBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorRespondToRenameFace(const Json::Value& config);
  
public:
  virtual bool WantsToBeActivatedBehavior() const override;
  
protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {}
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  virtual void OnBehaviorActivated()   override;
  virtual void HandleWhileInScopeButNotActivated(const EngineToGameEvent& event) override;
  
private:
  
  std::string      _name;
  Vision::FaceID_t _faceID;
  AnimationTrigger _animTrigger; // set by Json config
  
}; // class BehaviorReactToRenameFace
  

} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorRespondToRenameFace_H__
