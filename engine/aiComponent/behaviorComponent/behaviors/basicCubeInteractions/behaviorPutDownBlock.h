/**
 * File: behaviorPutDownBlock.h
 *
 * Author: Brad Neuman
 * Created: 2016-05-23
 *
 * Description: Simple behavior which puts down a block (using an animation group)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPutDownBlock_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPutDownBlock_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

// forward declerations
class CarryingComponent;

class BehaviorPutDownBlock : public ICozmoBehavior
{
public:

  // TODO: Use a PlaceObjectOnGround action (with animatino) and use its VisuallyVerify
  // helper for creating an action to make sure that the "put down" animation is working. It looks down at the
  // block to make sure we have a chance to see it
  static IActionRunner* CreateLookAfterPlaceAction(CarryingComponent& carryingComponent, bool doLookAtFaceAfter=true);

protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPutDownBlock(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}
  virtual bool WantsToBeActivatedBehavior() const override;

  virtual void OnBehaviorActivated() override;
  
private:
  struct InstanceConfig {
    InstanceConfig();
  };

  struct DynamicVariables {
    DynamicVariables();
  };


  void LookDownAtBlock();

};

} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorPutDownBlock_H__
