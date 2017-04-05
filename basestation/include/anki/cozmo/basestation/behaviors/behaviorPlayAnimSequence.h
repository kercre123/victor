/**
 * File: BehaviorPlayAnimSequence
 *
 * Author: Mark Wesley
 * Created: 11/03/15
 *
 * Description: Simple Behavior to play an animation or animation sequence
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlayAnimSequence_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlayAnimSequence_H__

#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorPlayAnimSequence : public IBehavior
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPlayAnimSequence(Robot& robot, const Json::Value& config, bool triggerRequired = true);
  
public:
  
  virtual ~BehaviorPlayAnimSequence();
  
  virtual bool IsRunnableInternal(const BehaviorPreReqNone& preReqData) const override;
  virtual bool IsRunnableInternal(const BehaviorPreReqAnimSequence& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override { return true;}
  
protected:
  
  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override { }

  // don't allow resume
  virtual Result ResumeInternal(Robot& robot) override { return RESULT_FAIL; }
  
  // queues actions to play all the animations specified in _animTriggers
  void StartSequenceLoop(Robot& robot);
  
  // ========== Members ==========
  
  mutable std::vector<AnimationTrigger> _animTriggers;
  int _numLoops;
  int _sequenceLoopsDone; // for sequences it's not per animation, but per sequence, so we have to wait till the last one
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlayAnimSequence_H__
