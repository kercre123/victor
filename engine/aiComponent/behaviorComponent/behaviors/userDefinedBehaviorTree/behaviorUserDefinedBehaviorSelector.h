/**
 * File: BehaviorUserDefinedBehaviorSelector.h
 *
 * Author: Hamzah Khan
 * Created: 2018-07-17
 *
 * Description: A behavior that enables a user to select a reaction to a condition that becomes the new default behavior to respond to that trigger.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorUserDefinedBehaviorSelector__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorUserDefinedBehaviorSelector__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorTextToSpeechLoop.h"
#include "engine/aiComponent/behaviorComponent/behaviors/robotDrivenDialog/behaviorPromptUserForVoiceCommand.h"

#include <queue>

#include "clad/types/behaviorComponent/behaviorIDs.h"
#include "clad/types/behaviorComponent/beiConditionTypes.h"


namespace Anki {
namespace Vector {

class BehaviorUserDefinedBehaviorSelector : public ICozmoBehavior
{
public: 
  virtual ~BehaviorUserDefinedBehaviorSelector();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorUserDefinedBehaviorSelector(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.behaviorAlwaysDelegates = false;
  };
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {};
  
  virtual void InitBehavior() override;
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;

private:
  // text to speech delegation helper function
  IBehavior* SetUpSpeakingBehavior(const std::string&);

  void CancelDelegatesAndSelf();
  void DemoBehaviorCallback();
  void PromptYesOrNoCallback();
  void DemoBehaviorRouter();

  struct InstanceConfig {
    InstanceConfig();
    std::shared_ptr<BehaviorTextToSpeechLoop> ttsBehavior;
    std::shared_ptr<BehaviorPromptUserForVoiceCommand> confirmationBehavior;
  };

  struct DynamicVariables {
    DynamicVariables();
    std::queue<BehaviorID> demoQueue;
    BehaviorID currentBehaviorId;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorUserDefinedBehaviorSelector__
