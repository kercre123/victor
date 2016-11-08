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
#include "anki/cozmo/basestation/blockWorld/blockConfiguration.h"
#include "anki/cozmo/basestation/blockWorld/stackConfigurationContainer.h"
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
  virtual bool CarryingObjectHandledInternally() const override {return false;}
  
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
  virtual void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot) override;
  void HandleObjectUpAxisChanged(const ObjectUpAxisChanged& msg, Robot& robot);
  
private:
  // TODO:(bn) a few behaviors have used this pattern now, maybe we should re-think having some kind of
  // UpdateWhileNotRunning
  mutable BlockConfigurations::StackWeakPtr _currentTallestStack;

  uint8_t _minStackHeight;
  
  // store block IDs until unknown block exposure issues are fixed
  ObjectID _bottomBlockID;
  ObjectID _middleBlockID;
  ObjectID _topBlockID;
  
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

  void TransitionToReachingForBlock(Robot& robot);
  void TransitionToKnockingOverStack(Robot& robot);
  void TransitionToBlindlyFlipping(Robot& robot);
  void TransitionToPlayingReaction(Robot& robot);
  
  void LoadConfig(const Json::Value& config);
  bool InitializeMemberVars();
  virtual void ClearStack();
  virtual void UpdateTargetStack(const Robot& robot) const;
  
  void PrepareForKnockOverAttempt();

};


}
}


#endif
