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

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include <memory>

namespace Anki {

class Pose3d;

namespace Cozmo {

class BlockWorldFilter;

class BehaviorDriveInDesperation : public ICozmoBehavior
{
  using Base = ICozmoBehavior;
public:
  virtual ~BehaviorDriveInDesperation();

protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorDriveInDesperation(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOffTreads = true;
  }

  virtual void InitBehavior() override;

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;
  
  virtual bool WantsToBeActivatedBehavior() const override;

private:
  using super = ICozmoBehavior;
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
  
  void TransitionToIdle();
  void TransitionToDriveRandom();
  void TransitionToRequest();

  // states only used with cubes
  void TransitionToSearchForCube();
  void TransitionToDriveToCube();
  void TransitionToLookAtCube();

  // Figure out which state to go to after idle is complete
  void TransitionFromIdle();

  void TransitionFromIdle_WithCubes();
  void TransitionFromIdle_NoCubes();

  void GetRandomDrivingPose(Pose3d& outPose);

  void RandomizeNumDrivingRounds();

  void SetState_internal(State state, const std::string& stateName);

};

}
}

#endif
