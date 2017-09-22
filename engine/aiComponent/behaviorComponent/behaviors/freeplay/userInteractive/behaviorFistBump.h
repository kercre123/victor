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

#include "engine/aiComponent/behaviorComponent/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {
  
class IFistBumpListener;
  
class BehaviorFistBump : public IBehavior
{
private:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorFistBump(const Json::Value& config);
  
public:

  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override {return true;}
  
protected:
  
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual void AddListener(IFistBumpListener* listener) override;

private:
  
  enum class State {
    PutdownObject,
    LookForFace,
    LookingForFace,
    RequestInitialFistBump,
    RequestingFistBump,
    WaitingForMotorsToSettle,
    WaitingForBump,
    CompleteSuccess,
    CompleteFail,
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

  // Wait for bump vars
  f32 _waitStartTime_s;
  int _fistBumpRequestCnt;
  f32 _waitingAccelX_mmps2;
  
  // Recorded position of lift for bump detection
  f32 _liftWaitingAngle_rad;
  
  // Abort when picked up for long enough
  f32 _lastTimeOffTreads_s;
  
  // Whether or not to update the last time the FistBump completed for the
  // purpose of affecting trigger cooldown times
  bool _updateLastCompletionTime;

  std::set<IFistBumpListener*> _fistBumpListeners;
  void ResetTrigger(bool updateLastCompletionTime);
  
}; // class BehaviorFistBump
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorFistBump_H__
