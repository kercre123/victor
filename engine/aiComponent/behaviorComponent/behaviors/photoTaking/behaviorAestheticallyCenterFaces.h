/**
 * File: BehaviorAestheticallyCenterFaces.h
 *
 * Author: Kevin M. Karol
 * Created: 2018-06-04
 *
 * Description: Centers face/s so that they will look pleasing in an image - this may not be true center if aesthetics dictate more space should be left on the top/bottom of the frame
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorAestheticallyCenterFaces__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorAestheticallyCenterFaces__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorAestheticallyCenterFaces : public ICozmoBehavior
{
public: 
  virtual ~BehaviorAestheticallyCenterFaces();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorAestheticallyCenterFaces(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
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

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorAestheticallyCenterFaces__
