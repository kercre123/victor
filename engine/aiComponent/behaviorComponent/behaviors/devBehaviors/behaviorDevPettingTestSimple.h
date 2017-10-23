/**
 * File: BehaviorDevPettingTestSimple.h
 *
 * Author: Arjun Menon
 * Date:   09/12/2017
 *
 * Description: simple test behavior to respond to touch
 *              and petting input. Does nothing until a
 *              touch event comes in for it to react to
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorDevPettingTestSimple_H__
#define __Cozmo_Basestation_Behaviors_BehaviorDevPettingTestSimple_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorDevPettingTestSimple : public ICozmoBehavior
{
public:
  
  virtual ~BehaviorDevPettingTestSimple() { }
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override { return false; }
  
protected:
  
  friend class BehaviorContainer;
  BehaviorDevPettingTestSimple(const Json::Value& config);
  
  virtual void HandleWhileActivated(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void   OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;


  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;
  
private:
  
  void HandleRobotTouched(BehaviorExternalInterface& behaviorExternalInterface, const EngineToGameEvent& msg);
  
  // last received touch event time
  double _lastTouchTime_s;
  
  // last time reacting to touch (used to rate limit reactions)
  double _lastReactTime_s;
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorDevPettingTestSimple_H__
