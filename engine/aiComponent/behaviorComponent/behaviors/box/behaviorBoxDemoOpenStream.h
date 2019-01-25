/**
 * File: BehaviorBoxDemoOpenStream.h
 *
 * Author: Brad
 * Created: 2019-01-24
 *
 * Description: Open a stream to chipper for intents based on a touch event
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBoxDemoOpenStream__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBoxDemoOpenStream__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorBoxDemoOpenStream : public ICozmoBehavior
{
public: 
  virtual ~BehaviorBoxDemoOpenStream();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorBoxDemoOpenStream(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBoxDemoOpenStream__
