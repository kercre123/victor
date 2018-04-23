/**
 * File: behaviorConfirmObject.h
 *
 * Author: ross
 * Created: 2018-04-18
 *
 * Description: Runs only if an object position is known for a given object type(s), drives to where
 *              it should be, and ends when it (or any other object of the given type[s]) is seen. If it
 *              is not seen and there are multiple objects of the given type(s) in blockworld, it will
 *              sequentially try them all starting with the closest
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorConfirmObject__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorConfirmObject__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {
  
class ConditionObjectKnown;

class BehaviorConfirmObject : public ICozmoBehavior
{
public:
  virtual ~BehaviorConfirmObject();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorConfirmObject(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  virtual void InitBehavior() override;
  virtual void OnBehaviorEnteredActivatableScope() override;
  virtual void OnBehaviorLeftActivatableScope() override;
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:
  
  // drives to the closest object in _dVars.sortedObjects
  void DriveToClosestKnownObject();

  struct InstanceConfig {
    InstanceConfig();
    std::unique_ptr<ConditionObjectKnown> objectKnownCondition;
    std::unique_ptr<ConditionObjectKnown> objectKnownRecentlyCondition;
    std::unique_ptr<ConditionObjectKnown> objectJustSeen;
    int maxAttempts;
  };

  struct DynamicVariables {
    DynamicVariables();
    bool shouldActivate;
    bool shouldCancel;
    std::vector<const ObservableObject*> sortedObjects;
    int numAttempts;
  };
  
  InstanceConfig _iConfig;
  DynamicVariables _dVars;

};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorConfirmObject__

