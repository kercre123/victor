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

#ifndef __Cozmo_Basestation_Behaviors_BehaviorMultiRobotFistBump_H__
#define __Cozmo_Basestation_Behaviors_BehaviorMultiRobotFistBump_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {
  
class BEIRobotInfo;
class IFistBumpListener;
  
class BehaviorMultiRobotFistBump : public ICozmoBehavior
{
private:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorMultiRobotFistBump(const Json::Value& config);
  
public:

  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override {return true;}
  
protected:
  virtual void InitBehavior(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual void HandleWhileActivated(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  void HandleMultiRobotInteractionStateTransition(const ExternalInterface::MultiRobotInteractionStateTransition& msg, BehaviorExternalInterface& behaviorExternalInterface);
  void HandleMultiRobotSessionEnded(const ExternalInterface::MultiRobotSessionEnded& msg, BehaviorExternalInterface& behaviorExternalInterface);


  virtual void AddListener(IFistBumpListener* listener) override;

private:
  
  enum class State {
    PutdownObject = 0,
    GreetSlave,
    ApproachSlave,
    LookForFace,
    LookingForFace,
    RequestInitialFistBump,
    RequestingFistBump,
    WaitingForMotorsToSettle,
    WaitingForBump,
    PrepareToBumpFist,
    BumpFist,
    DrivingIntoFist,
    CompleteSuccess,
    CompleteFail,
    Complete
  };
  
  bool CheckForBump(const BEIRobotInfo& robotInfo);
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


  bool _sessionEnded;

  MultiRobotComponent* _mrc;
  bool _isMaster;
  State _partnerState;
  void SetState_internal(State state, const std::string& stateName);
  
  State _pendingState;
  State _gatingPartnerState;
  std::string _pendingStateName;


  u32 _unknownPoseCount;
  
  // Transitions to the specified state when _partnerState
  // reaches the specified state
  void SetStateWhenPartnerState(State state, const std::string& stateName, State partnerState);
  
}; // class BehaviorFistBump
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorMultiRobotFistBump_H__
