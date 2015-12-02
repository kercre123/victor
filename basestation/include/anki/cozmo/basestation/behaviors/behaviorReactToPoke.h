/**
 * File: behaviorReactToPoke.h
 *
 * Author: Kevin
 * Created: 11/30/15
 *
 * Description: Behavior for immediately responding to being poked
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToPoke_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToPoke_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/common/basestation/objectIDs.h"
#include <vector>

namespace Anki {
namespace Cozmo {

class BehaviorReactToPoke : public IReactionaryBehavior
{
public:
  BehaviorReactToPoke(Robot& robot, const Json::Value& config);
  
  virtual bool IsRunnable(const Robot& robot, double currentTime_sec) const override;
  
protected:
  
  virtual Result InitInternal(Robot& robot, double currentTime_sec, bool isResuming) override;
  virtual Status UpdateInternal(Robot& robot, double currentTime_sec) override;
  virtual Result InterruptInternal(Robot& robot, double currentTime_sec, bool isShortInterrupt) override;
  
private:
  enum class State {
    Inactive,
    IsPoked,
    PlayingAnimation
  };
  
  State _currentState = State::Inactive;
  bool _doReaction = false;
  u32 _animTagToWaitFor = 0;
  
  virtual void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot) override;
  
}; // class BehaviorReactToPoke
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToPoke_H__
