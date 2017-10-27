/**
* File: behaviorFeedingEat.h
*
* Author: Kevin M. Karol
* Created: 2017-3-28
*
* Description: Behavior for cozmo to interact with an "energy" filled cube
* and drain the energy out of it
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_Behaviors_Feeding_BehaviorFeedingEat_H__
#define __Cozmo_Basestation_Behaviors_Feeding_BehaviorFeedingEat_H__

#include "engine/behaviorSystem/behaviors/iBehavior.h"
#include "engine/behaviorSystem/feedingCubeController.h"
#include "engine/components/bodyLightComponent.h"


#include "anki/common/basestation/objectIDs.h"

#include <vector>

namespace Anki {
namespace Cozmo {
  
namespace CubeAccelListeners {
class MovementListener;
}

class BehaviorFeedingEat : public IBehavior
{
protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;  
  BehaviorFeedingEat(Robot& robot, const Json::Value& config);

public:
  virtual bool IsRunnableInternal(const Robot& robot) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}
  
  virtual void AddListener(IFeedingListener* listener) override{
    _feedingListeners.push_back(listener);
  };
  
  void SetTargetObject(const ObjectID& objID){_targetID = objID;}
  
protected:
  using base = IBehavior;
  virtual Result InitInternal(Robot& robot) override;

  // This behavior can't be resumed (cube might not be valid and the None/Robot version of IsRunnable isn't
  // implemented)
  virtual Result ResumeInternal(Robot& robot) override { return Result::RESULT_FAIL; }

  virtual Status UpdateInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;
  
private:
  enum class State{
    ReactingToInterruption,
    DrivingToFood,
    PlacingLiftOnCube,
    Eating
  };
  
  mutable ObjectID _targetID;
  float _timeCubeIsSuccessfullyDrained_sec;
  bool  _hasNotifiedNeedsSystemOfDrain;

  State _currentState;
  
  // Listeners which should be notified when Cozmo starts eating
  std::vector<IFeedingListener*> _feedingListeners;
  // Listen for the cube being pulled away from Cozmo
  std::shared_ptr<CubeAccelListeners::MovementListener> _cubeMovementListener; // CubeAccelComponent listener

  // map of cubes to skip. Key is the object ID, value is the last pose updated timestamp where we want to
  // ignore it. If the pose has updated after this timestamp, consider it valid again
  std::map< ObjectID, TimeStamp_t > _badCubesMap;
  
  void TransitionToDrivingToFood(Robot& robot);
  void TransitionToPlacingLiftOnCube(Robot& robot);
  void TransitionToEating(Robot& robot);
  void TransitionToReactingToInterruption(Robot& robot);

  void CubeMovementHandler(Robot& robot, const float movementScore);
  AnimationTrigger CheckNeedsStateAndCalculateAnimation(Robot& robot);
  
  bool DidSuccessfullyDrainCube();

  // sets the target cube as invalid for future runs of the behavior (unless it is observed again);
  void MarkCubeAsBad(const Robot& robot);
  bool IsCubeBad(const Robot& robot, const ObjectID& objectID) const;
  
  void SetState_internal(State state, const std::string& stateName);
  
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_Behaviors_Feeding_BehaviorFeedingEat_H__
