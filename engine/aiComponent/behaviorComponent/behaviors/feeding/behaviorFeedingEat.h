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

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/feedingCubeController.h"
#include "engine/components/bodyLightComponent.h"


#include "anki/common/basestation/objectIDs.h"

#include <vector>

namespace Anki {
namespace Cozmo {
  
namespace CubeAccelListeners {
class MovementListener;
}

class BehaviorFeedingEat : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;  
  BehaviorFeedingEat(const Json::Value& config);

public:
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}
  
  virtual void AddListener(IFeedingListener* listener) override{
    _feedingListeners.insert(listener);
  };

  // remove a listener if preset and return true. Otherwise, return false
  bool RemoveListeners(IFeedingListener* listener);

  void SetTargetObject(const ObjectID& objID){_targetID = objID;}
  
protected:
  using base = ICozmoBehavior;
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void   OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  
private:
  enum class State{
    ReactingToInterruption,
    DrivingToFood,
    PlacingLiftOnCube,
    Eating
  };
  
  mutable ObjectID _targetID;
  float _timeCubeIsSuccessfullyDrained_sec;
  bool  _hasRegisteredActionComplete;

  State _currentState;
  
  // Listeners which should be notified when Cozmo starts eating
  std::set<IFeedingListener*> _feedingListeners;
  // Listen for the cube being pulled away from Cozmo
  std::shared_ptr<CubeAccelListeners::MovementListener> _cubeMovementListener; // CubeAccelComponent listener

  // map of cubes to skip. Key is the object ID, value is the last pose updated timestamp where we want to
  // ignore it. If the pose has updated after this timestamp, consider it valid again
  std::map< ObjectID, TimeStamp_t > _badCubesMap;
  
  void TransitionToDrivingToFood(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToPlacingLiftOnCube(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToEating(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToReactingToInterruption(BehaviorExternalInterface& behaviorExternalInterface);

  void CubeMovementHandler(BehaviorExternalInterface& behaviorExternalInterface, const float movementScore);
  AnimationTrigger CheckNeedsStateAndCalculateAnimation(BehaviorExternalInterface& behaviorExternalInterface);

  // sets the target cube as invalid for future runs of the behavior (unless it is observed again);
  void MarkCubeAsBad(BehaviorExternalInterface& behaviorExternalInterface);
  bool IsCubeBad(BehaviorExternalInterface& behaviorExternalInterface, const ObjectID& objectID) const;
  
  void SetState_internal(State state, const std::string& stateName);
  
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_Behaviors_Feeding_BehaviorFeedingEat_H__
