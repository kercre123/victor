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

#include "engine/aiComponent/behaviorSystem/behaviors/iBehavior.h"
#include "engine/aiComponent/behaviorSystem/AIBeacon.h"
#include "engine/aiComponent/AIWhiteboard.h"

#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/objectIDs.h"

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
class BehaviorExploreBringCubeToBeacon : public IBehavior
{
private:

  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
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
  // IBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  virtual bool IsRunnableInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override { return true;}

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // State functions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void TransitionToPickUpObject(BehaviorExternalInterface& behaviorExternalInterface, int attempt);   // state when we are ready to go pick up a cube
  void TransitionToObjectPickedUp(BehaviorExternalInterface& behaviorExternalInterface); // state when the object is picked up
  
protected:
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // find cube in the beacon to stack the one we have on top of the former
  const ObservableObject* FindFreeCubeToStackOn(const ObservableObject* object, const AIBeacon* beacon, const BehaviorExternalInterface& behaviorExternalInterface) const;
  
  // find pose to drop the object inside the selected beacon. Return true/false on success/failure
  static bool FindFreePoseInBeacon(const ObservableObject* object, const AIBeacon* selectedBeacon, const BehaviorExternalInterface& behaviorExternalInterface, Pose3d& freePose, float recentFailureCooldown_sec);
  
  // helper to simplify code. Returns object addressed by index in the _candidateObjects vector, null if not a valid entry
  const ObservableObject* GetCandidate(const BlockWorld& world, size_t index) const;
  
  // generate the proper action and callback to try to stack the cube we are carrying on top of the given one
  void TryToStackOn(BehaviorExternalInterface& behaviorExternalInterface, const ObjectID& bottomCubeID, int attempt);
  
  // generate the proper action and callback to try to place the cube we are carrying at the given location
  void TryToPlaceAt(BehaviorExternalInterface& behaviorExternalInterface, const Pose3d& pose, int attempt);
  
  // Fires appropriate emotion events depending on if a cube was placed at a beacon or all cubes are in beacon
  void FireEmotionEvents(BehaviorExternalInterface& behaviorExternalInterface);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // list of objects selected in IsRunnable. Cached as a performance optimization
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
