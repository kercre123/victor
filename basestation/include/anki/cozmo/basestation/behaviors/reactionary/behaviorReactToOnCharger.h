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

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

namespace Anki {
namespace Cozmo {

class BehaviorReactToOnCharger : public IReactionaryBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToOnCharger(Robot& robot, const Json::Value& config);
  
public:
  
  virtual bool IsRunnableInternalReactionary(const Robot& robot) const override;
  virtual bool ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event, const Robot& robot) override;
  virtual bool ShouldResumeLastBehavior() const override { return false; }
  
protected:
  
  virtual Result InitInternalReactionary(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  
  virtual void HandleWhileRunning(const GameToEngineEvent& event, Robot& robot) override;
  
  virtual void StopInternalReactionary(Robot& robot) override;
  
private:
  
  bool _isOnCharger = false;
  float _timeTilSleepAnimation_s = -1.0;
  float _timeTilDisconnect_s = 0.0;
  
  // prevent Cozmo from asking to go to sleep for a period of time after connection
  mutable float _dontRunUntilTime_sec;
  
}; // class BehaviorReactToOnCharger
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToOnCharger_H__
