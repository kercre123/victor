/**
 * File: BehaviorShowText.h
 *
 * Author: Brad
 * Created: 2019-01-07
 *
 * Description: Display fixed (or programatically set) text for a given time
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorShowText__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorShowText__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorShowText : public ICozmoBehavior
{
public: 
  virtual ~BehaviorShowText();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorShowText(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:

  struct InstanceConfig {
    InstanceConfig();

    // see IBehaviorSelfTest::DrawTextOnScreen docs for the meaning of this vector
    std::vector<std::string> text;

    float timeout_s;
  };

  struct DynamicVariables {
    DynamicVariables();

    float cancelTime_s = -1.0f;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorShowText__
