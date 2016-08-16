/**
 * File: behaviorPounceOnMotion.h
 *
 * Author: Brad Neuman
 * Created: 2015-11-18
 *
 * Description: This is a reactionary behavior which "pounces". Basically, it looks for motion nearby in the
 *              ground plane, and then drive to drive towards it and "catch" it underneath it's lift
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPounceOnMotion_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPounceOnMotion_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

namespace Anki {
namespace Cozmo {

class BehaviorPounceOnMotion : public IBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPounceOnMotion(Robot& robot, const Json::Value& config);
  
public:
  virtual bool IsRunnableInternal(const Robot& robot) const override;
  virtual float EvaluateScoreInternal(const Robot& robot) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}

protected:

  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
  virtual void HandleWhileNotRunning(const EngineToGameEvent& event, const Robot& robot) override;

  virtual Result InitInternal(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;

  float _maxPounceDist = 120.0f;
  float _minGroundAreaForPounce = 0.01f;
  float _maxTimeBetweenPoses = 4.0f;
  
  
  float _prePouncePitch = 0.0f;
  float _lastValidPouncePoseTime = 0.0f;
  int _numValidPouncePoses = 0;

  float _lastPoseDist = 0.0f;
  
  // Overwritten by config.
  float _maxTimeSinceNoMotion_running_sec = 30.0;
  float _maxTimeSinceNoMotion_notRunning_sec = 30.0;
  float _boredomMultiplier = 0.8f;
  float _maxTimeBeforeRotate = 5.f;
  
  
private:

  enum class State {
    Inactive,
    InitialAnim,
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

  float _lastTimeRotate;
  float _lastMotionTime;
  State _state = State::Inactive;

  u32 _waitForActionTag = 0;

  bool  _cliffReactEnabled = true;
  float _stopRelaxingTime = 0.0f;
  float _backUpDistance = 0.f;
  float GetDriveDistance();
  void  EnableCliffReacts(bool enable,Robot& robot);

  // reset everything for when the behavior is finished
  void Cleanup(Robot& robot);
  
  void SetState_internal(State state, const std::string& stateName);
  void TransitionToInitialWarningAnim(Robot& robot);
  void TransitionToBringingHeadDown(Robot& robot);
  void TransitionToRotateToWatchingNewArea(Robot& robot);
  void TransitionToWaitForMotion(Robot& robot);
  void TransitionToTurnToMotion(Robot& robot, int16_t motion_img_x, int16_t motion_img_y);
  void TransitionToCreepForward(Robot& robot);
  void TransitionToPounce(Robot& robot);
  void TransitionToRelaxLift(Robot& robot);
  void TransitionToResultAnim(Robot& robot);
  void TransitionToBackUp(Robot& robot);
  void TransitionToGetOutBored(Robot& robot);
};

}
}

#endif
