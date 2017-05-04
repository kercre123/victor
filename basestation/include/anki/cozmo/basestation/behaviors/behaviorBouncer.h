/**
 * File: behaviorBouncer.h
 *
 * Description: Cozmo plays a classic retro arcade game
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorBouncer_H__
#define __Cozmo_Basestation_Behaviors_BehaviorBouncer_H__

#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/smartFaceId.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorBouncer : public IBehavior
{
public:
  
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorBouncer(Robot& robot, const Json::Value& config);
  
  // IBehavior interface
  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReq) const override;
  virtual bool IsRunnableInternal(const BehaviorPreReqAcknowledgeFace& preReq) const override;
  virtual bool CarryingObjectHandledInternally() const override { return false; }
  
  virtual Result InitInternal(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  virtual void StopInternal(Robot& robot) override;
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
  
private:
  
  // Helper types
  enum class State {
    Init,                   // Initialize everything and play starting animations
    Timeout,                // Timeout expired
    Complete
  };
  
  // Member variables
  State _state = State::Init;
  
  // This must be mutable to retain state from trigger prerequisites
  mutable SmartFaceID _target;
  
  // When did behavior start running?
  float _startedAt_sec;
  
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorBouncer_H__
