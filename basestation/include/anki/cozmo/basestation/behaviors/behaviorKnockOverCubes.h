/**
 * File: behaviorKnockOverCubes.h
 *
 * Author: Kevin M. Karol
 * Created: 2016-08-01
 *
 * Description: Behavior to tip over a stack of cubes
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorKnockOverCubes_H__
#define __Cozmo_Basestation_Behaviors_BehaviorKnockOverCubes_H__

#include "anki/common/basestation/objectIDs.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {

class BlockWorldFilter;
class ObservableObject;
class Robot;

class BehaviorKnockOverCubes : public IBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorKnockOverCubes(Robot& robot, const Json::Value& config);

  virtual Result InitInternal(Robot& robot) override;
  virtual Result ResumeInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;

  virtual bool IsRunnableInternal(const Robot& robot) const override;  
  
private:

  // TODO:(bn) a few behaviors have used this pattern now, maybe we should re-think having some kind of
  // UpdateWhileNotRunning
  mutable ObjectID _baseBlockID;
  mutable uint8_t _stackHeight;
  
  uint8_t _minStackHeight;
  
  enum class State {
    DrivingToStack,
    ReachingForBlock,
    DrivingToReadyPose,
    TurningTowardsFace,
    AligningWithStack,
    KnockingOverStack,
    PlayingReaction,
    SettingDownBlock
  };

  State _state = State::DrivingToStack;
  
  //Values loaded in from JSON
  AnimationTrigger _reachForBlockTrigger = AnimationTrigger::Count;
  AnimationTrigger _putDownAnimTrigger = AnimationTrigger::Count;
  AnimationTrigger _knockOverEyesTrigger = AnimationTrigger::Count;
  AnimationTrigger _knockOverSuccessTrigger = AnimationTrigger::Count;
  bool _shouldStreamline;

  void TransitionToReachingForBlock(Robot& robot);
  void TransitionToDrivingToReadyPose(Robot& robot);
  void TransitionToTurningTowardsFace(Robot& robot);
  void TransitionToAligningWithStack(Robot& robot);
  void TransitionToKnockingOverStack(Robot& robot);
  void TransitionToPlayingReaction(Robot& robot);
  void TranstitionToSettingDownBlock(Robot& robot);
  
  void LoadConfig(const Json::Value& config);
  virtual void ResetBehavior(Robot& robot);
  virtual void UpdateTargetStack(const Robot& robot) const;
  void SetState_internal(State state, const std::string& stateName);

};


}
}


#endif
