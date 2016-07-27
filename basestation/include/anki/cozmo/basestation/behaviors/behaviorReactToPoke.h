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

// NOTE: this behavior is deprecated. We may bring it back, but it is currently disabled and not up to date

#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToPoke_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToPoke_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/common/basestation/objectIDs.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include <vector>

namespace Anki {
namespace Cozmo {

class BehaviorReactToPoke : public IReactionaryBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToPoke(Robot& robot, const Json::Value& config);
  
public:
  
  virtual bool IsRunnableInternalReactionary(const Robot& robot) const override;
  virtual bool ShouldResumeLastBehavior() const override { return true; }
  
protected:
  
  virtual Result InitInternalReactionary(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  virtual void   StopInternalReactionary(Robot& robot) override;
  
  virtual void AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot) override;

  
private:
  
  // Cap on how long ago we had to have observed a face in order to turn towards it
  constexpr static TimeStamp_t kMaxTimeSinceLastObservedFace_ms = 120000;
  
  enum class State {
    Inactive,
    ReactToPoke,
    TurnToFace,
    ReactToFace
  };
  
  State _currentState = State::Inactive;
  bool _doReaction = false;
  u32 _lastActionTag = 0;
  bool _isActing = false;
  
  
  void StartActing(Robot& robot, IActionRunner* action);
}; // class BehaviorReactToPoke
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToPoke_H__
