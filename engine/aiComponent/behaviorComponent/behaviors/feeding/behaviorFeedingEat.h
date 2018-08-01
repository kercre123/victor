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
#include "engine/components/backpackLights/backpackLightComponent.h"


#include "coretech/common/engine/objectIDs.h"
#include "coretech/common/engine/robotTimeStamp.h"

#include <vector>

namespace Anki {
namespace Cozmo {
  
namespace CubeAccelListeners {
class MovementListener;
}

class BehaviorFeedingEat : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorFeedingEat(const Json::Value& config);

public:
  virtual bool WantsToBeActivatedBehavior() const override;
  
  virtual void AddListener(IFeedingListener* listener) override{
    _iConfig.feedingListeners.insert(listener);
  };

  // remove a listener if preset and return true. Otherwise, return false
  bool RemoveListeners(IFeedingListener* listener);

  void SetTargetObject(const ObjectID& objID){_dVars.targetID = objID;}
  
protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {}
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}

  using base = ICozmoBehavior;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

  virtual void OnBehaviorDeactivated() override;
  
private:
  enum class State{
    ReactingToInterruption,
    DrivingToFood,
    PlacingLiftOnCube,
    Eating
  };

  struct InstanceConfig {
    InstanceConfig();
    // Listeners which should be notified when Cozmo starts eating
    std::set<IFeedingListener*> feedingListeners;
    // Listen for the cube being pulled away from Cozmo
    std::shared_ptr<CubeAccelListeners::MovementListener> cubeMovementListener; // CubeAccelComponent listener
  };

  struct DynamicVariables {
    DynamicVariables();

    State currentState;
    mutable ObjectID targetID;

    float timeCubeIsSuccessfullyDrained_sec;
    bool  hasRegisteredActionComplete;
    // map of cubes to skip. Key is the object ID, value is the last pose updated timestamp where we want to
    // ignore it. If the pose has updated after this timestamp, consider it valid again
    std::map< ObjectID, RobotTimeStamp_t > badCubesMap;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;
  
  void TransitionToDrivingToFood();
  void TransitionToPlacingLiftOnCube();
  void TransitionToEating();
  void TransitionToReactingToInterruption();

  void CubeMovementHandler(const float movementScore);

  // sets the target cube as invalid for future runs of the behavior (unless it is observed again);
  void MarkCubeAsBad();
  bool IsCubeBad(const ObjectID& objectID) const;
  
  void SetState_internal(State state, const std::string& stateName);
  
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_Behaviors_Feeding_BehaviorFeedingEat_H__
