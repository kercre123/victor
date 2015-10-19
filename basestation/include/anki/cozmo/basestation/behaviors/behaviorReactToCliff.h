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
#include "util/signals/simpleSignal_fwd.h"
#include <vector>

namespace Anki {
namespace Cozmo {

// Forward declaration
template<typename TYPE> class AnkiEvent;
namespace ExternalInterface { class MessageEngineToGame; }
  
class BehaviorReactToCliff : public IReactionaryBehavior
{
public:
  BehaviorReactToCliff(Robot& robot, const Json::Value& config);
  
  virtual bool IsRunnable(double currentTime_sec) const override;
  
  virtual Result Init(double currentTime_sec) override;
  
  virtual Status Update(double currentTime_sec) override;
  
  // Finish placing current object if there is one, otherwise good to go
  virtual Result Interrupt(double currentTime_sec) override;
  
  virtual bool GetRewardBid(Reward& reward) override;
  
private:
  enum class State {
    Inactive,
    CliffDetected,
    PlayingAnimation
    };
  
  State _currentState;
  bool _cliffDetected = false;
  bool _waitingForAnimComplete = false;
  u32 _animTagToWaitFor = 0;
  
  std::vector<Signal::SmartHandle> _eventHandles;
  
  void HandleCliffEvent(const AnkiEvent<ExternalInterface::MessageEngineToGame>& event);
}; // class BehaviorReactToCliff
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToCliff_H__
