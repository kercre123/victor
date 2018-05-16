/**
 * File: behaviorDanceToTheBeat.h
 *
 * Author: Matt Michini
 * Created: 2018-05-07
 *
 * Description: Dance to a beat detected by the microphones/beat detection algorithm
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_Behaviors_BehaviorDanceToTheBeat_H__
#define __Engine_Behaviors_BehaviorDanceToTheBeat_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorDanceToTheBeat : public ICozmoBehavior
{
  
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorDanceToTheBeat(const Json::Value& config);
  
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.behaviorAlwaysDelegates = false;
  };
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  
private:
  using base = ICozmoBehavior;
  
  struct InstanceConfig {
    InstanceConfig(const Json::Value& config, const std::string& debugName);
    const float danceDuration_sec;
    const float danceDurationVariability_sec;
  };
  
  struct DynamicVariables {
    float nextAnimTime_sec = 0.f;
    float beatPeriod_sec = 0.f;
    float dancingEndTime_sec = 0.f;
    bool getoutAnimPlayed = false;
  };
  
  InstanceConfig   _iConfig;
  DynamicVariables _dVars;
  
};


} // namespace Cozmo
} // namespace Anki


#endif // __Engine_Behaviors_BehaviorDanceToTheBeat_H__

