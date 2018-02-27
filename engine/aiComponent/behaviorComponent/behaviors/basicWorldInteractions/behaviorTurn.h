/**
 * File: behaviorTurn.h
 *
 * Author:  Kevin M. Karol
 * Created: 1/26/18
 *
 * Description:  Behavior which turns the robot a set number of degrees
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorTurn_H__
#define __Cozmo_Basestation_Behaviors_BehaviorTurn_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorTurn : public ICozmoBehavior
{
protected:  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorTurn(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {}

  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

  virtual bool WantsToBeActivatedBehavior() const override;
  
  
private:
  struct {
    float turnRad = 0.f;
  } _params;

}; // class __Cozmo_Basestation_Behaviors_BehaviorTurn_H__

} // namespace Cozmo
} // namespace Anki

#endif
