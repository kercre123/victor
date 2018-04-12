/**
 * File: behaviorReactToUnexpectedMovement.h
 *
 * Author: Al Chaussee
 * Created: 7/11/2016
 *
 * Description: Behavior for reacting to unexpected movement like being spun while moving
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToUnexpectedMovement_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToUnexpectedMovement_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/beiConditions/conditions/conditionUnexpectedMovement.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/types/unexpectedMovementTypes.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorReactToUnexpectedMovement : public ICozmoBehavior
{
private:
  using super = ICozmoBehavior;
  
  friend class BehaviorFactory;
  BehaviorReactToUnexpectedMovement(const Json::Value& config);
  
  ConditionUnexpectedMovement _unexpectedMovementCondition;
  
public:  
  virtual bool WantsToBeActivatedBehavior() const override;
  
protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
  }
  virtual void GetBehaviorJsonKeys( std::set<const char*>& expectedKeys ) const override {}

  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override { };
};
  
}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToUnexpectedMovement_H__
