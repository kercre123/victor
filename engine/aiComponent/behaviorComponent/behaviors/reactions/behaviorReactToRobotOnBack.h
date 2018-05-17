/**
 * File: behaviorReactToRobotOnBack.h
 *
 * Author: Brad Neuman
 * Created: 2016-05-06
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToRobotOnBack_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToRobotOnBack_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "engine/aiComponent/beiConditions/conditions/conditionOffTreadsState.h"

namespace Anki {
namespace Cozmo {

class BehaviorReactToRobotOnBack : public ICozmoBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToRobotOnBack(const Json::Value& config);
  
public:
  virtual bool WantsToBeActivatedBehavior() const override;

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
    modifiers.wantsToBeActivatedWhenOffTreads = true;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}

  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;

private:

  ConditionOffTreadsState _offTreadsCondition;
  
  void FlipDownIfNeeded();
  void DelayThenFlipDown();
  
};

}
}

#endif
