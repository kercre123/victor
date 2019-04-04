/**
 * File: BehaviorSimpleVoiceResponse.h
 *
 * Author: Brad
 * Created: 2019-03-29
 *
 * Description: A behavior that can perform a simple response to a number of different voice commands based on
 * the simple response mapping in user_intents.json
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorSimpleVoiceResponse__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorSimpleVoiceResponse__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorSimpleVoiceResponse : public ICozmoBehavior
{
public:
  virtual ~BehaviorSimpleVoiceResponse();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorSimpleVoiceResponse(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {};

  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;

};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorSimpleVoiceResponse__
