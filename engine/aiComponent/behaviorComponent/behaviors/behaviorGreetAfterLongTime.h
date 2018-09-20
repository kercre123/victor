/**
 * File: BehaviorGreetAfterLongTime.h
 *
 * Author: Hamzah Khan
 * Created: 2018-06-25
 *
 * Description: This behavior causes victor to, upon seeing a face, react "loudly" if he hasn't seen the person for a certain amount of time.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorGreetAfterLongTime__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorGreetAfterLongTime__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "coretech/common/engine/robotTimeStamp.h"

#include "osState/wallTime.h"

#include <string>

namespace Anki {
namespace Vector {


class BehaviorGreetAfterLongTime : public ICozmoBehavior
{
public:
  virtual ~BehaviorGreetAfterLongTime();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorGreetAfterLongTime(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::High });
  };
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual void InitBehavior() override;
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:
  // plays the desired reaction animation
  void PlayReactionAnimation();

  // runs checks to decide whether the behavior update should run during this tick
  bool ShouldBehaviorUpdate(time_t&);

  struct InstanceConfig {
    InstanceConfig();

    // length of time before robot will react strongly to a known face
    uint cooldownPeriod_s;
  };

  struct DynamicVariables {
    DynamicVariables();
    std::shared_ptr<std::unordered_map<std::string, time_t>> lastSeenTimesPtr;

    // last time a face was seen
    RobotTimeStamp_t lastFaceCheckTime_ms;

    // should play reaction flag
    bool shouldActivateBehavior;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorGreetAfterLongTime__
