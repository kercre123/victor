/**
 * File: behaviorNone.h
 *
 * Author: Lee Crippen
 * Created: 10/01/15
 *
 * Description: Behavior to do nothing.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorNone_H__
#define __Cozmo_Basestation_Behaviors_BehaviorNone_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorNone: public IBehavior
{
public:
  BehaviorNone(Robot& robot, const Json::Value& config) : IBehavior(robot, config)
  {
    _name = "NoneBehavior";
    
    // Baseline emotion score so this behavior gets a non-zero score regardless of mood
    AddEmotionScorer(EmotionScorer(EmotionType::Excited, Anki::Util::GraphEvaluator2d({{0.0f, 0.05f}}), false));
  }
  virtual ~BehaviorNone() { }
  
  //
  // Abstract methods to be overloaded:
  //
  virtual bool IsRunnable(const Robot& robot, double currentTime_sec) const override { return true; }
  
protected:
  
  virtual Result InitInternal(Robot& robot, double currentTime_sec) override
  {
    _isInterrupted = false; return Result::RESULT_OK;
  }
  
  virtual Status UpdateInternal(Robot& robot, double currentTime_sec) override
  {
    Status retval = _isInterrupted ? Status::Complete : Status::Running;
    return retval;
  }

  virtual Result InterruptInternal(Robot& robot, double currentTime_sec) override
  {
    _isInterrupted = true; return Result::RESULT_OK;
  }
  
  bool _isInterrupted = false;
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorNone_H__
