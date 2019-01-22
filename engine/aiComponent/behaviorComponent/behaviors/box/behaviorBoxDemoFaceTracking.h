/**
 * File: behaviorBoxDemoFaceTracking.h
 *
 * Author: Andrew Stein
 * Created: 2019-01-04
 *
 * Description: Demo of face tracking for The Box demo
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBoxDemoFaceTracking__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBoxDemoFaceTracking__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorBoxDemoFaceTracking : public ICozmoBehavior
{
public: 
  virtual ~BehaviorBoxDemoFaceTracking();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorBoxDemoFaceTracking(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:

  struct InstanceConfig {
    InstanceConfig();
    
    bool playSoundOnNewFace = false;
    
    // TODO: put configuration variables here
  };

  struct DynamicVariables {
    DynamicVariables();
    
    // TODO: put member variables here
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBoxDemoFaceTracking__
