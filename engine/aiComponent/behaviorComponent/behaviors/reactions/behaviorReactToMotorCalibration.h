/**
 * File: BehaviorReactToMotorCalibration.h
 *
 * Author: Kevin Yoon
 * Created: 11/2/2016
 *
 * Description: Behavior for reacting to automatic motor calibration
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToMotorCalibration_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToMotorCalibration_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Vector {

  
class BehaviorReactToMotorCalibration : public ICozmoBehavior
{
private:
  using super = ICozmoBehavior;
  
  friend class BehaviorFactory;
  BehaviorReactToMotorCalibration(const Json::Value& config);
  
public:  
  virtual bool WantsToBeActivatedBehavior() const override;

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}
  
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override { };

  virtual void HandleWhileActivated(const EngineToGameEvent& event) override;

  constexpr static f32 _kTimeout_sec = 5.;
  
};
  
}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToMotorCalibration_H__
