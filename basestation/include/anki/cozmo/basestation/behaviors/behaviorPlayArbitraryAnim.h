/**
 * File: BehaviorPlayArbitraryAnim.h
 *
 * Author: Kevin M. Karol
 * Created: 08/17/16
 *
 * Description: Behavior that can be used to play an arbitrary animation computationally
 * Should not be used as type for a behavior created from JSONs - should be created on demand
 * with the factory and owned by the chooser creating them so that other parts of the system
 * don't re-set the animation trigger in a race condition
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlayArbitraryAnim_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlayArbitraryAnim_H__

#include "anki/cozmo/basestation/behaviors/behaviorPlayAnim.h"
#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorPlayArbitraryAnim: public BehaviorPlayAnim
{
using BaseClass = BehaviorPlayAnim;
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPlayArbitraryAnim(Robot& robot, const Json::Value& config);

public:
  
  virtual ~BehaviorPlayArbitraryAnim();
  void SetAnimationTrigger(AnimationTrigger trigger, int numLoops);
  
  virtual bool IsRunnableInternal(const Robot& robot) const override;
  
protected:
  virtual Result InitInternal(Robot& robot) override;
  
private:
  bool _animationAlreadySet;
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlayArbitraryAnim_H__
