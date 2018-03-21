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

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {
  
class BEIRobotInfo;
class IFistBumpListener;
  
class BehaviorFistBump : public ICozmoBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorFistBump(const Json::Value& config);
  
public:
  virtual bool WantsToBeActivatedBehavior() const override;  

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
    modifiers.behaviorAlwaysDelegates = false;
    modifiers.wantsToBeActivatedWhenOnCharger = false;
    modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::Low });
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;


  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;
  
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

  struct InstanceConfig {
    InstanceConfig(const Json::Value& config);
    f32  maxTimeToLookForFace_s;
    bool abortIfNoFaceFound;

    // Whether or not to update the last time the FistBump completed for the
    // purpose of affecting trigger cooldown times
    bool updateLastCompletionTime;
    std::set<IFistBumpListener*> fistBumpListeners;
  };

  struct DynamicVariables {
    DynamicVariables();
    State state;
    // Looking for faces vars
    f32  startLookingForFaceTime_s;
    f32  nextGazeChangeTime_s;
    u32  nextGazeChangeIndex;
      // Wait for bump vars
    f32 waitStartTime_s;
    int fistBumpRequestCnt;
    f32 waitingAccelX_mmps2;
      // Recorded position of lift for bump detection
    f32 liftWaitingAngle_rad;
    
    // Abort when picked up for long enough
    f32 lastTimeOffTreads_s;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;

  
  bool CheckForBump(const BEIRobotInfo& robotInfo);
  void ResetTrigger(bool updateLastCompletionTime);
  
  void ResetFistBumpTimer() const;
  
}; // class BehaviorFistBump
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorFistBump_H__
