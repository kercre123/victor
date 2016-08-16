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

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/behaviorSystem/AIBeacon.h"

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
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorExploreBringCubeToBeacon(Robot& robot, const Json::Value& config);
  
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // destructor
  virtual ~BehaviorExploreBringCubeToBeacon() override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  virtual bool IsRunnableInternal(const Robot& robot) const override;  
  virtual bool CarryingObjectHandledInternally() const override { return true;}

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // State functions
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void TransitionToPickUpObject(Robot& robot);   // state when we are ready to go pick up a cube
  void TransitionToObjectPickedUp(Robot& robot); // state when the object is picked up
  
protected:
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  virtual Result InitInternal(Robot& robot) override;
  virtual void StopInternal(Robot& robot) override;
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // find cube in the beacon to stack the one we have on top of the former
  const ObservableObject* FindFreeCubeToStackOn(const ObservableObject* object, const AIBeacon* beacon, const Robot& robot) const;
  
  // find pose to drop the object inside the selected beacon. Return true/false on success/failure
  static bool FindFreePoseInBeacon(const ObservableObject* object, const AIBeacon* selectedBeacon, const Robot& robot, Pose3d& freePose);
  
  // helper to simplify code. Returns object addressed by index in the _candidateObjects vector, null if not a valid entry
  const ObservableObject* GetCandidate(const BlockWorld& world, size_t index) const;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // list of objects selected in IsRunnable. Cached as a performance optimization
  struct CandidateInfo {
    CandidateInfo(ObjectID objId, ObjectFamily fam) : id(objId), family(fam) {}
    ObjectID id;
    ObjectFamily family;
  };
  mutable std::vector<CandidateInfo> _candidateObjects;
  
  // cubes on which we have failed to stack, so that we don't try to restack on them
  std::set<ObjectID> _invalidCubesToStackOn;
  
  // store ID in case something happen to the object while we move there
  ObjectID _selectedObjectID;
};
  

} // namespace Cozmo
} // namespace Anki

#endif //
