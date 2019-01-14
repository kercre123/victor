/**
 * File: BehaviorBoxDemoShowMicData.h
 *
 * Author: Brad
 * Created: 2019-01-14
 *
 * Description: Displays microphone data on robot (through face info screen currently)
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBoxDemoShowMicData__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBoxDemoShowMicData__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorBoxDemoShowMicData : public ICozmoBehavior
{
public: 
  virtual ~BehaviorBoxDemoShowMicData();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorBoxDemoShowMicData(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;

  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}

  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBoxDemoShowMicData__
