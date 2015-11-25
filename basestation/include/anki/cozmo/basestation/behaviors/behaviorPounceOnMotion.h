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
public:

  BehaviorPounceOnMotion(Robot& robot, const Json::Value& config);

  // checks if the motion is within pouncing distance
  virtual bool IsRunnable(const Robot& robot, double currentTime_sec) const override;

  // this can only run if it detects some motion
  virtual float EvaluateScoreInternal(const Robot& robot, double currentTime_sec) const override;

protected:

  virtual void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot) override;
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;

  virtual Result InitInternal(Robot& robot, double currentTime_sec, bool isResuming) override;
  virtual Status UpdateInternal(Robot& robot, double currentTime_sec) override;
  virtual Result InterruptInternal(Robot& robot, double currentTime_sec, bool isShortInterrupt) override;

  float _maxPounceDist;
  float _minGroundAreaForPounce;
  float _prePouncePitch;

  float _lastValidPouncePose;
  int _numValidPouncePoses;
  
private:

  enum class State {
    Inactive,
    Pouncing,
    RelaxingLift,
    PlayingFinalReaction,
    Complete,
  };

  State _state;

  u32 _waitForActionTag;

  float _stopRelaxingTime;
  
  void CheckPounceResult(Robot& robot);

  // reset everything for when the behavior is finished
  void Cleanup(Robot& robot);

};

}
}

#endif
