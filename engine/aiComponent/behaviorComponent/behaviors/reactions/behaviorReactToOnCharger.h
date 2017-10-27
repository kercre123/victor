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

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorReactToOnCharger : public ICozmoBehavior
{
private:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorReactToOnCharger(const Json::Value& config);
  
public:
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override {return true;}
  virtual bool ShouldRunWhileOnCharger() const override { return true;}

  // true so that we can handle edge cases where the robot is on the charger and not on his treads
  virtual bool ShouldRunWhileOffTreads() const override { return true;}
  
protected:
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual void HandleWhileActivated(const GameToEngineEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  
private:
  bool _onChargerCanceled;
  float _timeTilSleepAnimation_s;
  float _timeTilDisconnect_s;
  bool _triggerableFromVoiceCommand;

}; // class BehaviorReactToOnCharger
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToOnCharger_H__
