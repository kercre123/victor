/**
 * File: ${class_name}.h
 *
 * Author: $author
 * Created: $date
 *
 * Description: $description
 *
 * Copyright: Anki, Inc. $year
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_${class_name}__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_${class_name}__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class $class_name : public ICozmoBehavior
{
public: 
  virtual ~${class_name}();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit ${class_name}(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override ;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:

  struct InstanceConfig {
    InstanceConfig();
    // TODO: put configuration variables here
  };

  struct DynamicVariables {
    DynamicVariables();
    // TODO: put member variables here
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_${class_name}__
