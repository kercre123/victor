/**
 * File: BehaviorDevBatteryLogging.h
 *
 * Author: Kevin Yoon
 * Date:   04/06/2018
 *
 * Description: Behavior for logging battery data under a variety of test conditions
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorDevBatteryLogging_H__
#define __Cozmo_Basestation_Behaviors_BehaviorDevBatteryLogging_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {
  
class IBEICondition;
  
class BehaviorDevBatteryLogging : public ICozmoBehavior
{
public:
  
  virtual ~BehaviorDevBatteryLogging() { }
  virtual bool WantsToBeActivatedBehavior() const override;
  
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = false;
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.wantsToBeActivatedWhenOnCharger = true;
    modifiers.behaviorAlwaysDelegates = false;
  }
  
protected:

  using BExtI = BehaviorExternalInterface;
  
  friend class BehaviorFactory;
  BehaviorDevBatteryLogging(const Json::Value& config);
  
  void InitBehavior() override;
  
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override; 

  virtual void OnBehaviorActivated() override;

  virtual void OnBehaviorDeactivated() override;
  
  virtual void BehaviorUpdate() override;

private:

  struct InstanceConfig {
    InstanceConfig();
    f32 wheelSpeed_mmps;
    f32 liftSpeed_radps;
    f32 headSpeed_radps;

    f32 startMovingVoltage;

    bool stressWifi;
    bool stressCPU;
    bool stressSpeaker;
  };

  InstanceConfig   _iConfig;

  void InitLog();
  void LogData() const;

  void EnqueueMotorActions();
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorDevBatteryLogging_H__
