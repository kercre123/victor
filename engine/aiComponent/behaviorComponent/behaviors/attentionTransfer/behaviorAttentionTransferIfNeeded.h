/**
 * File: BehaviorAttentionTransferIfNeeded.h
 *
 * Author: Brad
 * Created: 2018-06-19
 *
 * Description: This behavior will perform an attention transfer (aka "look at phone") animation and send the
 *              corresponding message to the app if needed. It can be configured to require an event happen X
 *              times in Y seconds to trigger the transfer, and if that hasn't been met it can either do a
 *              fallback animation or nothing.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorAttentionTransferIfNeeded__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorAttentionTransferIfNeeded__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "coretech/common/engine/utils/recentOccurrenceTracker.h"

#include "clad/types/behaviorComponent/attentionTransferTypes.h"

namespace Anki {
namespace Cozmo {

class BehaviorAttentionTransferIfNeeded : public ICozmoBehavior
{
public: 
  virtual ~BehaviorAttentionTransferIfNeeded();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorAttentionTransferIfNeeded(const Json::Value& config);  

  virtual void InitBehavior() override;
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOffTreads = true;
  }
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override {}
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override {
    return true;
  }
  
  virtual void OnBehaviorActivated() override;

private:

  struct InstanceConfig {
    AttentionTransferReason reason = AttentionTransferReason::Invalid;
    RecentOccurrenceTracker::Handle recentOccurrenceHandle;
    int numberOfTimes = 0;
    float amountOfSeconds = 0.0f;
    AnimationTrigger animIfNotRecent = AnimationTrigger::Count;
  };

  InstanceConfig _iConfig;

  void TransitionToAttentionTransfer();
  void TransitionToNoAttentionTransfer();

};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorAttentionTransferIfNeeded__
