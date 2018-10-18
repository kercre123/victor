/**
 * File: BehaviorReactToBrightColor.h
 *
 * Author: Patrick Doran
 * Created: 2018-10-16
 *
 * Description: React when a bright color is detected
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToBrightColor__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToBrightColor__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "coretech/common/engine/robotTimeStamp.h"

namespace Anki {
namespace Vector {

class BehaviorReactToBrightColor : public ICozmoBehavior
{
public: 
  virtual ~BehaviorReactToBrightColor();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorReactToBrightColor(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;
  virtual void InitBehavior() override;

private:

  struct InstanceConfig {
    InstanceConfig(const Json::Value& config);
    std::string eyeColorBehaviorStr;
    ICozmoBehaviorPtr eyeColorBehavior;
  };

  struct DynamicVariables {
    struct Persistent {
      RobotTimeStamp_t lastSeenTimeStamp;
    } persistent;

    DynamicVariables();
    void Reset(bool keepPersistent);

    //! The previous eye color to transition back to
    u32 prevEyeColor;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToBrightColor__
