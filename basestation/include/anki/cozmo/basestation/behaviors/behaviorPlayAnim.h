/**
 * File: behaviorPlayAnim
 *
 * Author: Mark Wesley
 * Created: 11/03/15
 *
 * Description: Simple Behavior to play an animation
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlayAnim_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlayAnim_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorPlayAnim: public IBehavior
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPlayAnim(Robot& robot, const Json::Value& config, bool keyRequired = true);
  
public:
  
  virtual ~BehaviorPlayAnim();
  
  virtual bool IsRunnableInternal(const Robot& robot) const override;
  virtual bool CarryingObjectHandledInternally() const override { return true;}
  
protected:
  
  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override { }
  
  // ========== Members ==========
  
  AnimationTrigger _animTrigger;
  int              _numLoops;
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlayAnim_H__
