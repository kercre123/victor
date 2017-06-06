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

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
#include "anki/common/basestation/objectIDs.h"
#include <vector>

namespace Anki {
namespace Cozmo {

class BehaviorReactToCliff : public IBehavior
{
private:
  using super = IBehavior;
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToCliff(Robot& robot, const Json::Value& config);
  
public:
  
  virtual bool IsRunnableInternal(const BehaviorPreReqNone& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override { return true;}
  
protected:
  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;
  
  virtual void HandleWhileNotRunning(const EngineToGameEvent& event, const Robot& robot) override;
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
  
  virtual Status UpdateInternal(Robot& robot) override;

private:
  using base = IBehavior;
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

  u16 _cliffDetectThresholdAtStart = 0;
  bool _quitReaction = false;
  
  bool _shouldStopDueToCharger;
  
  
}; // class BehaviorReactToCliff
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToCliff_H__
