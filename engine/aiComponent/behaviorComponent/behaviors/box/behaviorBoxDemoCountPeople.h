/**
 * File: BehaviorBoxDemoCountPeople.h
 *
 * Author: Andrew Stein
 * Created: 2019-01-14
 *
 * Description: Uses local neural net person classifier to trigger cloud object/person detection
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBoxDemoCountPeople__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBoxDemoCountPeople__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorBoxDemoCountPeople : public ICozmoBehavior
{
public: 
  virtual ~BehaviorBoxDemoCountPeople();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorBoxDemoCountPeople(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;

private:

  struct InstanceConfig {
    InstanceConfig();
    float visionRequestTimeout_sec = 10.f;
    float waitTimeBetweenImages_sec = 1.f;
  };

  struct DynamicVariables {
    DynamicVariables();
    RobotTimeStamp_t lastImageTime_ms = 0;
    int maxFacesSeen = 0; // during *each* run of this behavior
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
  void TransitionToWaitingForPeople();
  void RespondToAnyPeopleDetected();
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBoxDemoCountPeople__
