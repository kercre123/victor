/**
 * File: carryingComponent.h
 *
 * Author: Al Chaussee
 * Created: 6/13/2017
 *
 * Description: Component for managing carrying/lift related things
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Anki_Cozmo_Basestation_Components_CarryingComponent_H__
#define __Anki_Cozmo_Basestation_Components_CarryingComponent_H__

#include "coretech/common/engine/objectIDs.h"
#include "coretech/common/shared/types.h"

#include "coretech/vision/engine/visionMarker.h"
#include "coretech/vision/shared/MarkerCodeDefinitions.h"

#include "clad/types/robotStatusAndActions.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "engine/robotComponents_fwd.h"

#include "util/helpers/noncopyable.h"

namespace Anki {
namespace Cozmo {

class ObservableObject;
class Robot;

class CarryingComponent : public IDependencyManagedComponent<RobotComponentID>, private Util::noncopyable
{
public:
  CarryingComponent();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {};
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {};
  //////
  // end IDependencyManagedComponent functions
  //////
  
  // Send a message to the robot to place whatever it is carrying on the
  // ground right where it is. Returns RESULT_FAIL if robot is not carrying
  // anything.
  Result PlaceObjectOnGround();
  
  Result SendSetCarryState(CarryState state) const;
  
  // Transitions the object that robot was docking with to the one that it
  // is carrying, and puts it in the robot's pose chain, attached to the
  // lift. Returns RESULT_FAIL if the robot wasn't already docking with
  // an object.
  Result SetDockObjectAsAttachedToLift();
  
  // Places the object that the robot was carrying in its current position w.r.t. the world, and removes
  // it from the lift pose chain so it is no "attached" to the robot. Set deleteLocatedObject=true to delete
  // the object instead of leaving it at the pose
  Result SetCarriedObjectAsUnattached(bool deleteLocatedObjects = false);
  
  // TODO Define better API for (un)setting carried objects: they are confused easily with SetCarriedObjectAsUnattached
  void SetCarryingObject(ObjectID carryObjectID, Vision::Marker::Code atMarkerCode);
  void UnSetCarryingObjects(bool topOnly = false);
  
  // If objID == carryingObjectOnTopID, only that object's carry state is unset.
  // If objID == carryingObjectID, all carried objects' carry states are unset.
  void UnSetCarryObject(ObjectID objID);
  
  const ObjectID&            GetCarryingObject()      const {return _carryingObjectID;}
  const ObjectID&            GetCarryingObjectOnTop() const {return _carryingObjectOnTopID;}
  const std::set<ObjectID>   GetCarryingObjects()     const;
  const Vision::Marker::Code GetCarryingMarkerCode()  const {return _carryingMarkerCode;}
  
  bool IsCarryingObject() const {return _carryingObjectID.IsSet(); }
  bool IsCarryingObject(const ObjectID& objectID) const;
  
private:
  
  // Sets object with objectID as attached to lift pose
  Result SetObjectAsAttachedToLift(const ObjectID& objectID,
                                   const Vision::KnownMarker::Code atMarkerCode);
  
  Robot* _robot = nullptr;
  
  ObjectID                  _carryingObjectID;
  Vision::KnownMarker::Code _carryingMarkerCode = Vision::MARKER_INVALID;
  ObjectID                  _carryingObjectOnTopID;
  
};

}
}



#endif
