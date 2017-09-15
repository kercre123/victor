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

#include "engine/behaviorSystem/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorDevPettingTestSimple : public IBehavior
{
public:
  
  virtual ~BehaviorDevPettingTestSimple() { }
  virtual bool IsRunnableInternal(const Robot& robot) const override;
  virtual bool CarryingObjectHandledInternally() const override { return false; }
  
protected:
  
  friend class BehaviorContainer;
  BehaviorDevPettingTestSimple(Robot& robot, const Json::Value& config);
  
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;

  virtual Result InitInternal(Robot& robot) override;

  virtual void StopInternal(Robot& robot) override;
  
  virtual BehaviorStatus UpdateInternal(Robot& robot) override;
  
private:
  
  void HandleRobotTouched(const Robot& robot, const EngineToGameEvent& msg);
  
  // last received touch event time
  double _lastTouchTime_s;
  
  // last time reacting to touch (used to rate limit reactions)
  double _lastReactTime_s;
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorDevPettingTestSimple_H__
