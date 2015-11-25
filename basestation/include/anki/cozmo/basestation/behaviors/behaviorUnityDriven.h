/**
 * File: behaviorUnityDriven
 *
 * Author: Mark Wesley
 * Created: 11/17/15
 *
 * Description: Unity driven behavior - a wrapper that allows Unity to drive behavior asynchronously via CLAD messages
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorUnityDriven_H__
#define __Cozmo_Basestation_Behaviors_BehaviorUnityDriven_H__


#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"


namespace Anki {
namespace Cozmo {

  
class BehaviorUnityDriven : public IBehavior
{
public:
  
  BehaviorUnityDriven(Robot& robot, const Json::Value& config);
  virtual ~BehaviorUnityDriven();
  
  virtual bool IsRunnable(const Robot& robot, double currentTime_sec) const override { return _isRunnable; }
  
  virtual bool IsShortInterruption() const override { return _isShortInterruption; }
  
  virtual float EvaluateScoreInternal(const Robot& robot, double currentTime_sec) const override;
    
protected:
  
  virtual Result InitInternal(Robot& robot, double currentTime_sec, bool isResuming) override;
  virtual Status UpdateInternal(Robot& robot, double currentTime_sec) override;
  virtual Result InterruptInternal(Robot& robot, double currentTime_sec, bool isShortInterrupt) override;
  
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
  
private:
  
  float _externalScore;
  
  bool  _isShortInterruption;
  bool  _isScoredExternally;
  
  bool  _isRunnable;
  bool  _wasInterrupted;
  bool  _isComplete;
  bool  _isResuming;
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorUnityDriven_H__
