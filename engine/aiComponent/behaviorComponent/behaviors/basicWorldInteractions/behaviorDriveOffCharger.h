/**
 * File: behaviorDriveOffCharger.h
 *
 * Author: Molly Jameson
 * Created: 2016-05-19
 *
 * Description: Behavior to drive to the edge off a charger and deal with the firmware cliff stop
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorDriveOffCharger_H__
#define __Cozmo_Basestation_Behaviors_BehaviorDriveOffCharger_H__

#include "coretech/common/engine/math/pose.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorDriveOffCharger : public ICozmoBehavior
{
protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorDriveOffCharger(const Json::Value& config);

public:
  virtual bool WantsToBeActivatedBehavior() const override;

protected:
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override{
    modifiers.wantsToBeActivatedWhenOffTreads = true;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  enum class DebugState {
    DrivingForward,
  };
  
  
private:
  struct InstanceConfig {
    InstanceConfig();
    float distToDrive_mm;
  };

  struct DynamicVariables {
    DynamicVariables();
    bool pushedIdleAnimation;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;


  void TransitionToDrivingForward();
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_Behaviors_BehaviorDriveOffCharger_H__
