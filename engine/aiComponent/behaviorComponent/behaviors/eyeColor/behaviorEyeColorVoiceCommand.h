/**
 * File: BehaviorEyeColorVoiceCommand.h
 *
 * Author: Andrew Stout
 * Created: 2019-03-19
 *
 * Description: Voice command for changing Vector's eye color.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorEyeColorVoiceCommand__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorEyeColorVoiceCommand__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "engine/aiComponent/behaviorComponent/userIntentData.h"
#include "proto/external_interface/settings.pb.h"

namespace Anki {
namespace Vector {

class BehaviorEyeColorVoiceCommand : public ICozmoBehavior
{
public: 
  virtual ~BehaviorEyeColorVoiceCommand();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorEyeColorVoiceCommand(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:

  external_interface::EyeColor GetDesiredColorFromIntent(UserIntentPtr intentData);
  external_interface::EyeColor ChooseColorCyclic();
  bool SetEyeColor(external_interface::EyeColor desiredEyeColor);
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorEyeColorVoiceCommand__
