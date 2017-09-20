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

#include "engine/aiComponent/behaviorSystem/behaviors/iBehavior.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Cozmo {

class Robot;
  
class BehaviorReactToMotorCalibration : public IBehavior
{
private:
  using super = IBehavior;
  
  friend class BehaviorContainer;
  BehaviorReactToMotorCalibration(const Json::Value& config);
  
public:  
  virtual bool IsRunnableInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override {return true;}

protected:
  
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void   OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override { };

  virtual void HandleWhileRunning(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;

  constexpr static f32 _kTimeout_sec = 5.;
  
};
  
}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToMotorCalibration_H__
