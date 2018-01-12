/**
 * File: behaviorPounceOnMotion.h
 *
 * Author: Brad Neuman
 * Created: 2015-11-18
 *
 * Description: This is a behavior which "pounces". Basically, it looks for motion nearby in the
 *              ground plane, and then drive to drive towards it and "catch" it underneath it's lift
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPounceOnMotion_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPounceOnMotion_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorPounceOnMotion : public ICozmoBehavior
{
private:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPounceOnMotion(const Json::Value& config);
  
public:
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;

protected:
  virtual void AlwaysHandleInScope(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void HandleWhileActivated(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void HandleWhileInScopeButNotActivated(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {}

  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;

  float _maxPounceDist = 120.0f;
  float _minGroundAreaForPounce = 0.01f;
  float _maxTimeBetweenPoses = 4.0f;
  
  
  float _prePouncePitch = 0.0f;
  float _lastValidPouncePoseTime = 0.0f;
  int _numValidPouncePoses = 0;

  float _lastPoseDist = 0.0f;
  
  // used for tracking total turn angle
  Radians _searchAmplitude_rad = DEG_TO_RAD(90);
  Radians _cumulativeTurn_rad;
  
  // Overwritten by config.
  float _maxTimeSinceNoMotion_running_sec = 30.0f;
  float _maxTimeSinceNoMotion_notRunning_sec = 30.0f;
  float _maxTimeBehaviorTimeout_sec = 30.0f;
  float _boredomMultiplier = 0.8f;
  float _maxTimeBeforeRotate = 5.f;
  float _oddsOfPouncingOnTurn = 0.0f;
  
  
private:

  enum class State {
    Inactive,
    InitialPounce,
    InitialReaction,
    InitialSearch,
    BringingHeadDown,
    RotateToWatchingNewArea,
    WaitingForMotion,
    TurnToMotion,
    CreepForward,
    Pouncing,
    RelaxingLift,
    PlayingFinalReaction,
    BackUp,
    GetOutBored,
    Complete,
  };
  
  
  int16_t _observedX;
  int16_t _observedY;
  bool _motionObserved = false;

  float _lastTimeRotate;
  float _lastMotionTime;
  float _startedBehaviorTime_sec = 0.0f;
  State _state = State::Inactive;
  bool  _relaxedLift = false;
  bool  _skipGetOutAnim = false;
  
  float _backUpDistance = 0.f;
  float GetDriveDistance();
  
  // ensures we don't get stuck in an infinite pounce loop
  float _lastCliffEvent_sec;
  
  // running count of the number of times we've observed motion without pouncing
  int _motionObservedNoPounceCount;

  // set if we think a human has interacted, to decide whether to register needs action completed
  bool  _humanInteracted;
  
  // modeled off of DelegateIfInControl callbacks
  template<typename T>
  void PounceOnMotionWithCallback(BehaviorExternalInterface& behaviorExternalInterface, void(T::*callback)(BehaviorExternalInterface&), IActionRunner* intermittentAction = nullptr);

  // reset everything for when the behavior is finished
  void Cleanup(BehaviorExternalInterface& behaviorExternalInterface);
  
  void SetState_internal(State state, const std::string& stateName);
  void TransitionToInitialPounce(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToInitialReaction(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToInitialSearch(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToBringingHeadDown(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToRotateToWatchingNewArea(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToWaitForMotion(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionFromWaitForMotion(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToTurnToMotion(BehaviorExternalInterface& behaviorExternalInterface, int16_t motion_img_x, int16_t motion_img_y);
  void TransitionToCreepForward(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToPounce(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToResultAnim(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToBackUp(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToGetOutBored(BehaviorExternalInterface& behaviorExternalInterface);
  
  void InitHelper(BehaviorExternalInterface& behaviorExternalInterface);
  bool IsFingerCaught(BehaviorExternalInterface& behaviorExternalInterface);
  
};

}
}

#endif
