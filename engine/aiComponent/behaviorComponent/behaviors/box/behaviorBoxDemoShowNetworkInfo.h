/**
 * File: BehaviorBoxDemoShowNetworkInfo.h
 *
 * Author: Brad
 * Created: 2019-01-14
 *
 * Description: Show the network info (including IP address)
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBoxDemoShowNetworkInfo__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBoxDemoShowNetworkInfo__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorBoxDemoShowNetworkInfo : public ICozmoBehavior
{
public: 
  virtual ~BehaviorBoxDemoShowNetworkInfo();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorBoxDemoShowNetworkInfo(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;

  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}  

  bool _messageSent = false;
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBoxDemoShowNetworkInfo__
