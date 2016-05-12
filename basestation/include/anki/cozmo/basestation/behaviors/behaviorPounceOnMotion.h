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

protected:

  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
  virtual void HandleWhileNotRunning(const EngineToGameEvent& event, const Robot& robot) override;

  virtual Result InitInternal(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;

  float _maxPounceDist = 160.0f;
  float _minGroundAreaForPounce = 0.01f;
  float _maxTimeBetweenPoses = 4.0f;
  
  
  float _prePouncePitch = 0.0f;
  float _lastValidPouncePoseTime = 0.0f;
  int _numValidPouncePoses = 0;

  float _lastPoseDist = 0.0f;
  const float _driveForwardUntilDist = 75.0f;
  
  // Overwritten by config.
  float _maxTimeSinceNoMotion_sec = 30.0;
  float _backUpDistance = -50.0;
  
  
private:

  enum class State {
    Inactive,
    InitialAnim,
    BringingHeadDown,
    WaitingForMotion,
    Pouncing,
    RelaxingLift,
    PlayingFinalReaction,
    BackUp,
    Complete,
  };

  float _lastMotionTime;
  State _state = State::Inactive;

  u32 _waitForActionTag = 0;

  float _stopRelaxingTime = 0.0f;

  // reset everything for when the behavior is finished
  void Cleanup(Robot& robot);
  
  void TransitionToInitialWarningAnim(Robot& robot);
  void TransitionToBringingHeadDown(Robot& robot);
  void TransitionToWaitForMotion(Robot& robot);
  void TransitionToPounce(Robot& robot);
  void TransitionToRelaxLift(Robot& robot);
  void TransitionToResultAnim(Robot& robot);
  void TransitionToBackUp(Robot& robot);

};

}
}

#endif
