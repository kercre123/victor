/**
 * File: behaviorDriveInDesperation.h
 *
 * Author: Brad Neuman
 * Created: 2017-07-11
 *
 * Description: Behavior to randomly drive around in desperation, to be used in severe low needs states
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_Behaviors_Freeplay_BehaviorDriveInDesperation_H__
#define __Cozmo_Basestation_BehaviorSystem_Behaviors_Freeplay_BehaviorDriveInDesperation_H__

#include "engine/aiComponent/behaviorSystem/behaviors/iBehavior.h"
#include <memory>

namespace Anki {

class Pose3d;

namespace Cozmo {

class Robot;
class BlockWorldFilter;

class BehaviorDriveInDesperation : public IBehavior
{
  using Base = IBehavior;
public:
  virtual ~BehaviorDriveInDesperation();

protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorDriveInDesperation(const Json::Value& config);
  virtual void InitBehavior(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;

  // Do nothing while in the air, just idle
  virtual bool ShouldRunWhileOffTreads() const override { return true; } 
  
  virtual bool IsRunnableInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}

private:
  using super = IBehavior;
  enum class State {
    Idle,
    DriveRandom,
    SearchForCube,
    DriveToCube,
    LookAtCube,
    Request
  };

  struct Params;
  std::unique_ptr<const Params> _params;
  
  State _currentState = State::Idle;

  int _numTimesToDriveBeforeRequest = 0;

  float _nextTimeToRevisitCube = -1.0f;

  std::unique_ptr<BlockWorldFilter> _validCubesFilter;

  // if set, this is the current target cube we're interested in
  ObjectID _targetCube;

  bool _wasPickedUp = false;
  
  void TransitionToIdle(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToDriveRandom(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToRequest(BehaviorExternalInterface& behaviorExternalInterface);

  // states only used with cubes
  void TransitionToSearchForCube(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToDriveToCube(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToLookAtCube(BehaviorExternalInterface& behaviorExternalInterface);

  // Figure out which state to go to after idle is complete
  void TransitionFromIdle(BehaviorExternalInterface& behaviorExternalInterface);

  void TransitionFromIdle_WithCubes(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionFromIdle_NoCubes(BehaviorExternalInterface& behaviorExternalInterface);

  void GetRandomDrivingPose(const BehaviorExternalInterface& behaviorExternalInterface, Pose3d& outPose);

  void RandomizeNumDrivingRounds();

  void SetState_internal(State state, const std::string& stateName);

};

}
}

#endif
