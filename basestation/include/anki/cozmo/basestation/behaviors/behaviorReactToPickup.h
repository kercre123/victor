/**
 * File: behaviorReactToPickup.h
 *
 * Author: Lee
 * Created: 08/26/15
 *
 * Description: Behavior for immediately responding being picked up.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToPickup_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToPickup_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/common/basestation/objectIDs.h"
#include <vector>

namespace Anki {
namespace Cozmo {

class BehaviorReactToPickup : public IReactionaryBehavior
{
public:
  BehaviorReactToPickup(Robot& robot, const Json::Value& config);
  
  virtual bool IsRunnable(double currentTime_sec) const override;
  
protected:
  
  virtual Result InitInternal(Robot& robot, double currentTime_sec) override;
  virtual Status UpdateInternal(Robot& robot, double currentTime_sec) override;
  virtual Result InterruptInternal(Robot& robot, double currentTime_sec) override;
  
private:
  enum class State {
    Inactive,
    IsPickedUp,
    PlayingAnimation
  };
  
  State _currentState = State::Inactive;
  bool _isInAir = false;
  bool _waitingForAnimComplete = false;
  u32 _animTagToWaitFor = 0;
  
  virtual void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot) override;
  
}; // class BehaviorReactToPickup
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToPickup_H__
