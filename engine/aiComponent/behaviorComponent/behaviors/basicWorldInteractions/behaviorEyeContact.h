/**
* File: behaviorEyeContact.h
*
* Author: Robert Cosgriff
* Created: 2/5/18
*
* Description: Simple behavior to react to eye contact
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorEyeContact_H__
#define __Cozmo_Basestation_Behaviors_BehaviorEyeContact_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorEyeContact : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorEyeContact(const Json::Value& config) : ICozmoBehavior(config) {};
  
public:
  virtual ~BehaviorEyeContact() override {}
  virtual bool WantsToBeActivatedBehavior() const override;
  // virtual bool WantsToBeActivatedBehavior() const override
  // {
  // PRINT_NAMED_INFO("BehaviorEyeContact.WantsToBeActivatedBehavior","");
  //  return true;
  // }
  
protected:
  // TODO review modifiers
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.wantsToBeActivatedWhenOnCharger = true;
    // modifiers.behaviorAlwaysDelegates = false;
  }

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorEyeContact_H__
