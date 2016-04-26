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

namespace Anki {
namespace Cozmo {
  
class BehaviorPlayAnim: public IBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPlayAnim(Robot& robot, const Json::Value& config);
  
public:
  
  virtual ~BehaviorPlayAnim();
  
  virtual bool IsRunnable(const Robot& robot) const override;
  
protected:
  
  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override { }
  
private:
  
  void    PlayAnimation(Robot& robot, const std::string& animName);
  
  // ========== Members ==========
  
  std::string   _animationName;  
  int           _numLoops;
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlayAnim_H__
