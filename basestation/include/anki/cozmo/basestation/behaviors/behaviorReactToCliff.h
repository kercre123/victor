/**
 * File: behaviorReactToCliff.h
 *
 * Author: Kevin
 * Created: 10/16/15
 *
 * Description: Behavior for immediately responding to a detected cliff
 *
 * Copyright: Anki, Inc. 2015
 *
 **/
#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToCliff_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToCliff_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/common/basestation/objectIDs.h"
#include <vector>

namespace Anki {
namespace Cozmo {

class BehaviorReactToCliff : public IReactionaryBehavior
{
public:
  BehaviorReactToCliff(Robot& robot, const Json::Value& config);
  
  virtual bool IsRunnable(const Robot& robot, double currentTime_sec) const override;
  
protected:
    
  virtual Result InitInternal(Robot& robot, double currentTime_sec) override;
  virtual Status UpdateInternal(Robot& robot, double currentTime_sec) override;
  virtual Result InterruptInternal(Robot& robot, double currentTime_sec) override;
  
private:
  enum class State {
    Inactive,
    CliffDetected,
    PlayingAnimation
    };
  
  State _currentState = State::Inactive;
  bool _cliffDetected = false;
  bool _waitingForAnimComplete = false;
  u32 _animTagToWaitFor = 0;
  
  void HandleCliffEvent(const EngineToGameEvent& event);
}; // class BehaviorReactToCliff
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToCliff_H__
