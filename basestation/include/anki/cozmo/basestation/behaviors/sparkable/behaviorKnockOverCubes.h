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

class BehaviorKnockOverCubes : public IReactionaryBehavior
{
public:
  // Overridden to distinguish between the reactionary and spark versions of the behavior
  virtual bool IsReactionary() const override;
  
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorKnockOverCubes(Robot& robot, const Json::Value& config);

  virtual Result InitInternalReactionary(Robot& robot) override;
  virtual void   StopInternalReactionary(Robot& robot) override;
  virtual bool ShouldComputationallySwitch(const Robot& robot) override;
  
  virtual bool IsRunnableInternalReactionary(const Robot& robot) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}
  virtual bool ShouldResumeLastBehavior() const override { return false;}
  
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
  virtual void AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot) override;
  void HandleObjectUpAxisChanged(const ObjectUpAxisChanged& msg, Robot& robot);
  
private:

  // TODO:(bn) a few behaviors have used this pattern now, maybe we should re-think having some kind of
  // UpdateWhileNotRunning
  mutable ObjectID _baseBlockID;
  mutable uint8_t _stackHeight;
  mutable bool _objectObservedChanged;
  // For checking computational switch
  ObjectID _lastObservedObject;
  
  uint8_t _minStackHeight;
  
  enum class DebugState {
    DrivingToStack,
    ReachingForBlock,
    DrivingToReadyPose,
    TurningTowardsFace,
    AligningWithStack,
    KnockingOverStack,
    PlayingReaction,
    SettingDownBlock
  };
  


  int _numRetries;
  std::set<ObjectID> _objectsFlipped;
  
  //Values loaded in from JSON
  AnimationTrigger _reachForBlockTrigger = AnimationTrigger::Count;
  AnimationTrigger _putDownAnimTrigger = AnimationTrigger::Count;
  AnimationTrigger _knockOverEyesTrigger = AnimationTrigger::Count;
  AnimationTrigger _knockOverSuccessTrigger = AnimationTrigger::Count;
  AnimationTrigger _knockOverFailureTrigger = AnimationTrigger::Count;
  bool _isReactionary;

  void TransitionToReachingForBlock(Robot& robot);
  void TransitionToKnockingOverStack(Robot& robot);
  void TransitionToPlayingReaction(Robot& robot);
  
  void LoadConfig(const Json::Value& config);
  void InitializeMemberVars();
  virtual void ResetBehavior(Robot& robot);
  virtual void UpdateTargetStack(const Robot& robot) const;
  bool CheckIfRunnable() const;

};


}
}


#endif
