/**
 * File: behaviorReactToRobotShaken.h
 *
 * Author: Matt Michini
 * Created: 2017-01-11
 *
 * Description: Implementation of Dizzy behavior when robot is shaken
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BeahviorReactToRobotShaken_H__
#define __Cozmo_Basestation_Behaviors_BeahviorReactToRobotShaken_H__

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"

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
  virtual void StopInternal(Robot& robot) override;
  
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
  
  // The maximum filtered accelerometer magnitude encountered during the shaking event:
  float _maxShakingAccelMag = 0.f;

  float _shakingStartedTime_s = 0.f;
  float _shakenDuration_s = 0.f;

  // Possible Dizzy reactions:
  enum class EReaction {
    None,
    Soft,
    Medium,
    Hard,
    StillPickedUp
  };
  
  const char* EReactionToString(EReaction reaction) const;
  
  // The dizzy reaction that was played by this behavior:
  EReaction _reactionPlayed = EReaction::None;
  
};

}
}

#endif
