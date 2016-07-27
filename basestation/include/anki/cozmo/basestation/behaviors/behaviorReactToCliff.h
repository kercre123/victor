/**
 * File: behaviorReactToCliff.h
 *
 * Author: Kevin
 * Created: 10/16/15
 *
 * Description: Behavior for immediately responding to a detected cliff. This behavior actually handles both
 *              the stop and cliff events
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
private:
  using super = IReactionaryBehavior;
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToCliff(Robot& robot, const Json::Value& config);
  
public:
  
  virtual bool IsRunnableInternalReactionary(const Robot& robot) const override;
  virtual bool ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event, const Robot& robot) override;
  virtual bool ShouldResumeLastBehavior() const override { return true; }
  
protected:
  
  virtual Result InitInternalReactionary(Robot& robot) override;
  virtual void   StopInternalReactionary(Robot& robot) override;

  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;

private:

  enum class State {
    PlayingStopReaction,
    PlayingCliffReaction,
    BackingUp
  };

  State _state = State::PlayingStopReaction;

  bool _gotCliff = false;
  
  void TransitionToPlayingStopReaction(Robot& robot);
  void TransitionToPlayingCliffReaction(Robot& robot);
  void TransitionToBackingUp(Robot& robot);
  void SendFinishedReactToCliffMessage(Robot& robot);

  void SetState_internal(State state, const std::string& stateName);

}; // class BehaviorReactToCliff
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToCliff_H__
