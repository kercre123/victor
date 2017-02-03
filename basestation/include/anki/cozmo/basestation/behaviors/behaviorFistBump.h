/**
 * File: behaviorFistBump.cpp
 *
 * Author: Kevin Yoon
 * Date:   01/23/2017
 *
 * Description: Makes Cozmo fist bump with player
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorFistBump_H__
#define __Cozmo_Basestation_Behaviors_BehaviorFistBump_H__

#include "anki/cozmo/basestation/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorFistBump : public IBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorFistBump(Robot& robot, const Json::Value& config);
  
public:

  virtual bool IsRunnableInternal(const BehaviorPreReqNone& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}
  
protected:
  
  virtual Result InitInternal(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  virtual void StopInternal(Robot& robot) override;

private:
  
  enum class State {
    LookingForFace,
    RequestInitialFistBump,
    RequestingFistBump,
    WaitingForMotorsToSettle,
    WaitingForBump,
    Complete
  };
  
  bool CheckForBump(const Robot& robot);
  State _state;
  
  // Looking for faces vars
  f32  _startLookingForFaceTime_s;
  f32  _nextGazeChangeTime_s;
  u32  _nextGazeChangeIndex;
  f32  _maxTimeToLookForFace_s;
  bool _abortIfNoFaceFound;

  // Whether or not to do
  bool _doHesitatingInitialRequest;
  
  // Wait for bump vars
  f32 _waitStartTime_s;
  int _fistBumpRequestCnt;
  
  // Recorded position of lift for bump detection
  f32 _liftWaitingAngle_rad;
  
}; // class BehaviorFistBump
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorFistBump_H__
