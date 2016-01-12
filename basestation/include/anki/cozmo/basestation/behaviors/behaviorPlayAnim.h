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
  
  void SetAnimationName(const std::string& inName);
  
  void SetLoopCount(int newVal) { _loopCount = newVal; } // -ve means loop forever
  
  virtual bool IsRunnable(const Robot& robot, double currentTime_sec) const override;
  
  virtual bool IsShortInterruption() const override { return true; }
  
protected:
  
  virtual Result InitInternal(Robot& robot, double currentTime_sec, bool isResuming) override;
  virtual Status UpdateInternal(Robot& robot, double currentTime_sec) override;
  virtual Result InterruptInternal(Robot& robot, double currentTime_sec, bool isShortInterrupt) override;
  virtual void   StopInternal(Robot& robot, double currentTime_sec) override;
  
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
  
private:
  
  void    PlayAnimation(Robot& robot, const std::string& animName);
  
  // ========== Members ==========
  
  std::string   _animationName;
  
  int           _loopCount;

  int           _loopsLeft;
  u32           _lastActionTag;

  bool          _isInterruptable;
  bool          _isInterrupted;

  bool          _isActing;
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlayAnim_H__
