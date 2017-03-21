/**
 * File: behaviorReactToRobotShaken.h
 *
 * Author: Matt Michini
 * Created: 2017-01-11
 *
 * Description: Implementation of behavior when robot is shaken
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BeahviorReactToRobotShaken_H__
#define __Cozmo_Basestation_Behaviors_BeahviorReactToRobotShaken_H__

#include "anki/cozmo/basestation/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorReactToRobotShaken : public IBehavior
{
private:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToRobotShaken(Robot& robot, const Json::Value& config);
  
public:
  virtual bool IsRunnableInternal(const BehaviorPreReqNone& preReqData) const override { return true;}
  virtual bool ShouldRunWhileOffTreads() const override { return true;}
  virtual bool CarryingObjectHandledInternally() const override {return true;}
  
protected:
  
  virtual Result InitInternal(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;

private:

  // Main behavior states:
  enum class EState {
    Shaking,
    DoneShaking,
    WaitTilOnTreads,
    ActDizzy,
    Finished
  };
  
  EState _state = EState::Shaking;
  
  float _maxShakingAccelMag = 0.f;
  float _shakingStartedTime_s = 0.f;
  float _shakenDuration_s = 0.f;
  
  // Member constants:

  // Accelerometer magnitude threshold corresponding to "no longer shaking"
  static const float _kAccelMagnitudeShakingStoppedThreshold;
  
  // Dizzy factor thresholds for playing the soft, medium, or hard reactions
  static const float _kShakenDurationThresholdHard;
  static const float _kShakenDurationThresholdMedium;
  
};

}
}

#endif
