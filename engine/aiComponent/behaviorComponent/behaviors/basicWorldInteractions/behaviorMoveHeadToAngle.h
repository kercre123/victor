/**
 * File: BehaviorMoveHeadToAngle.h
 *
 * Author: ross
 * Created: 2018-05-17
 *
 * Description: Moves head to the given angle and exits
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorMoveHeadToAngle__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorMoveHeadToAngle__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorMoveHeadToAngle : public ICozmoBehavior
{
public: 
  virtual ~BehaviorMoveHeadToAngle();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorMoveHeadToAngle(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;

private:

  struct InstanceConfig {
    InstanceConfig();
    float headAngle_deg;
  };

  struct DynamicVariables {
    DynamicVariables();
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorMoveHeadToAngle__
