/**
 * File: behaviorReactToOnCharger.h
 *
 * Author: Molly
 * Created: 5/12/16
 *
 * Description: Behavior for going night night on charger
 *
 * Copyright: Anki, Inc. 2015
 *
 **/
#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToOnCharger_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToOnCharger_H__

#include "anki/cozmo/basestation/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorReactToOnCharger : public IBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToOnCharger(Robot& robot, const Json::Value& config);
  
public:
  virtual bool IsRunnableInternal(const BehaviorPreReqNone& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override {return true;}
  
protected:
  virtual Result InitInternal(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  
  virtual void HandleWhileRunning(const GameToEngineEvent& event, Robot& robot) override;
  virtual void StopInternal(Robot& robot) override;
  
private:
  bool _onChargerCanceled;
  float _timeTilSleepAnimation_s = -1.0;
  float _timeTilDisconnect_s = 0.0;
  

  
}; // class BehaviorReactToOnCharger
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToOnCharger_H__
