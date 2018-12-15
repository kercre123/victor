/**
 * File: BehaviorHowOldAreYou.h
 *
 * Author: Andrew Stout
 * Created: 2018-11-13
 *
 * Description: Responds to How Old Are You character voice intent.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorHowOldAreYou__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorHowOldAreYou__
#pragma once

#include "components/textToSpeech/textToSpeechTypes.h"
#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorCountingAnimation.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include <chrono>


namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BehaviorHowOldAreYou : public ICozmoBehavior
{
public: 
  virtual ~BehaviorHowOldAreYou();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorHowOldAreYou(const Json::Value& config);

  virtual void InitBehavior() override;
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;

private:

  struct InstanceConfig {
    InstanceConfig();
    // configuration variables here
    std::shared_ptr<BehaviorCountingAnimation> countingAnimationBehavior;
  };

  struct DynamicVariables {
    DynamicVariables();
    // member variables here
    std::chrono::hours robotAge_h;
    std::pair<int, std::string> robotAge_presentable;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  std::chrono::hours GetRobotAge();
  std::pair<int, std::string> PresentableAgeFromHours(std::chrono::hours age_dh);

};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorHowOldAreYou__
