/**
 * File: BehaviorPossiblePerformance.h
 *
 * Author: Brad
 * Created: 2019-04-23
 *
 * Description: Checks face requirement and a special long cooldown to decide if it's time to do the
 * (specified) performance
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPossiblePerformance__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPossiblePerformance__
#pragma once

#include "coretech/common/engine/robotTimeStamp.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

enum class AnimationTrigger : int32_t;

class BehaviorPossiblePerformance : public ICozmoBehavior
{
public: 
  virtual ~BehaviorPossiblePerformance();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorPossiblePerformance(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual void InitBehavior() override;
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:

  struct InstanceConfig {
    InstanceConfig();
    
    AnimationTrigger animTrigger;
    float cooldown_s;
    float probability;
    float rerollPeriod_s;
  };

  struct PersistentVariables {
    float onCooldownUntil_s = -1.0f;
    float nextProbRerollAfter_s = -1.0f;
    bool lastRollPassed = false;
  };

  RobotTimeStamp_t GetRecentFaceTime() const;

  InstanceConfig _iConfig;
  PersistentVariables _pVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPossiblePerformance__
