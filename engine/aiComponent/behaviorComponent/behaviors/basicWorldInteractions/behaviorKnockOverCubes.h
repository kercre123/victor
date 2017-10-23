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
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/blockWorld/blockConfiguration.h"
#include "engine/blockWorld/stackConfigurationContainer.h"
#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {

class BlockWorldFilter;
class ObservableObject;
class Robot;

class BehaviorKnockOverCubes : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorKnockOverCubes(const Json::Value& config);

  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual Result ResumeInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void   OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}
  
  virtual void HandleWhileActivated(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void AlwaysHandle(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  void HandleObjectUpAxisChanged(const ObjectUpAxisChanged& msg, BehaviorExternalInterface& behaviorExternalInterface);
  
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

  void TransitionToReachingForBlock(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToKnockingOverStack(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToBlindlyFlipping(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToPlayingReaction(BehaviorExternalInterface& behaviorExternalInterface);
  
  void LoadConfig(const Json::Value& config);
  bool InitializeMemberVars();
  virtual void ClearStack();
  virtual void UpdateTargetStack(BehaviorExternalInterface& behaviorExternalInterface) const;
  
  void PrepareForKnockOverAttempt();

};


}
}


#endif
