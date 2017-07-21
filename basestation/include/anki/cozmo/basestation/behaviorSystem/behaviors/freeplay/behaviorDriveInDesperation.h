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

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
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
  BehaviorDriveInDesperation(Robot& robot, const Json::Value& config);

  virtual Result InitInternal(Robot& robot) override;
  virtual void StopInternal(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;

  // Do nothing while in the air, just idle
  virtual bool ShouldRunWhileOffTreads() const override { return true; } 
  
  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const override;
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
  
  void TransitionToIdle(Robot& robot);
  void TransitionToDriveRandom(Robot& robot);
  void TransitionToRequest(Robot& robot);

  // states only used with cubes
  void TransitionToSearchForCube(Robot& robot);
  void TransitionToDriveToCube(Robot& robot);
  void TransitionToLookAtCube(Robot& robot);

  // Figure out which state to go to after idle is complete
  void TransitionFromIdle(Robot& robot);

  void TransitionFromIdle_WithCubes(Robot& robot);
  void TransitionFromIdle_NoCubes(Robot& robot);

  void GetRandomDrivingPose(const Robot& robot, Pose3d& outPose);

  void RandomizeNumDrivingRounds();

  void SetState_internal(State state, const std::string& stateName);

};

}
}

#endif
