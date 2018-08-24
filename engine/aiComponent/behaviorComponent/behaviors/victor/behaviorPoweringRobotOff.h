/**
 * File: BehaviorPoweringRobotOff.h
 *
 * Author: Kevin M. Karol
 * Created: 2018-07-18
 *
 * Description: Behavior which plays power on/off animations in response to the power button being held down
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPoweringRobotOff__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPoweringRobotOff__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

// forward declaration
class IBEICondition;

class BehaviorPoweringRobotOff : public ICozmoBehavior
{
public: 
  virtual ~BehaviorPoweringRobotOff();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorPoweringRobotOff(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual void InitBehavior() override;
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorEnteredActivatableScope() override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorLeftActivatableScope() override;

  virtual void AlwaysHandleInScope(const RobotToEngineEvent& event) override;


private:
  struct InstanceConfig {
    InstanceConfig(const Json::Value& config);
    std::shared_ptr<IBEICondition> activateBehaviorCondition;
    std::string powerOnAnimName;
    std::string powerOffAnimName;
    bool waitForAnimMsg;
  };

  struct DynamicVariables {
    DynamicVariables();
    bool waitingForAnimationCallback;
    TimeStamp_t timeLastPowerAnimStopped_ms;
    std::string lastAnimPlayedName;
    bool shouldStartPowerOffAnimaiton;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  void TransitionToPoweringOff();
  void TransitionToPoweringOn();
  
  void StartAnimation(const std::string& animName, const TimeStamp_t startTime_ms);
  TimeStamp_t GetLengthOfAnimation_ms(const std::string& animName);

};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPoweringRobotOff__
