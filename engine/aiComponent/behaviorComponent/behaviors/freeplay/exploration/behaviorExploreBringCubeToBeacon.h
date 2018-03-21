/**
 * File: behaviorExploreBringCubeToBeacon
 *
 * Author: Raul
 * Created: 04/14/16
 *
 * Description: Behavior to pick up and bring a cube to the current beacon.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_Behaviors_BehaviorExploreBringCubeToBeacon_H__
#define __Cozmo_Basestation_Behaviors_BehaviorExploreBringCubeToBeacon_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/aiBeacon.h"
#include "engine/aiComponent/aiWhiteboard.h"

#include "coretech/common/engine/math/pose.h"
#include "coretech/common/engine/objectIDs.h"

#include "clad/types/objectFamilies.h"

#include <vector>
#include <set>

namespace Anki {
namespace Cozmo {

// Forward declaration
namespace ExternalInterface {
struct RobotObservedObject;
}
class IAction;
class ObservableObject;
class AIBeacon;
class BlockWorld;
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// BehaviorExploreBringCubeToBeacon
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BehaviorExploreBringCubeToBeacon : public ICozmoBehavior
{
private:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorExploreBringCubeToBeacon(const Json::Value& config);
  
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  void LoadConfig(const Json::Value& config);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // destructor
  virtual ~BehaviorExploreBringCubeToBeacon() override;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ICozmoBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  virtual bool WantsToBeActivatedBehavior() const override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // State functions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void TransitionToPickUpObject(int attempt);   // state when we are ready to go pick up a cube
  void TransitionToObjectPickedUp(); // state when the object is picked up
  
protected:
  
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ICozmoBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // find cube in the beacon to stack the one we have on top of the former
  const ObservableObject* FindFreeCubeToStackOn(const ObservableObject* object, const AIBeacon* beacon) const;
  
  // find pose to drop the object inside the selected beacon. Return true/false on success/failure
  static bool FindFreePoseInBeacon(const ObservableObject* object, const AIBeacon* selectedBeacon, const BehaviorExternalInterface& behaviorExternalInterface, Pose3d& freePose, float recentFailureCooldown_sec);
  
  // helper to simplify code. Returns object addressed by index in the _candidateObjects vector, null if not a valid entry
  const ObservableObject* GetCandidate(const BlockWorld& world, size_t index) const;
  
  // generate the proper action and callback to try to stack the cube we are carrying on top of the given one
  void TryToStackOn(const ObjectID& bottomCubeID, int attempt);
  
  // generate the proper action and callback to try to place the cube we are carrying at the given location
  void TryToPlaceAt(const Pose3d& pose, int attempt);
  
  // Fires appropriate emotion events depending on if a cube was placed at a beacon or all cubes are in beacon
  void FireEmotionEvents();
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // list of objects selected in WantsToBeActivated. Cached as a performance optimization
  mutable AIWhiteboard::ObjectInfoList _candidateObjects;
  
  // store ID in case something happen to the object while we move there
  ObjectID _selectedObjectID;
  
  // Configurable attributes
  struct Configuration
  {
    // how long we care about recent failures. Cozmo normally shouldn't forget when things happen, but since
    // we don't have ways to reliable detect when conditions change, this has to be a sweet tweak between "not too often"
    // and "not forever"
    float   recentFailureCooldown_sec;
  };
  
  // parsed configurations params from json
  Configuration _configParams;
};
  

} // namespace Cozmo
} // namespace Anki

#endif //
