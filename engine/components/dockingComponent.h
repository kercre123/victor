/**
 * File: dockingComponent.h
 *
 * Author: Al Chaussee
 * Created: 6/12/2017
 *
 * Description: Component for managing docking related things
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Anki_Cozmo_Basestation_Components_DockingComponent_H__
#define __Anki_Cozmo_Basestation_Components_DockingComponent_H__

#include "coretech/common/engine/objectIDs.h"
#include "coretech/common/engine/robotTimeStamp.h"

#include "coretech/vision/engine/visionMarker.h"
#include "coretech/vision/shared/MarkerCodeDefinitions.h"

#include "clad/types/dockingSignals.h"
#include "clad/types/robotStatusAndActions.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "engine/robotComponents_fwd.h"

#include "util/helpers/noncopyable.h"

namespace Anki {
namespace Cozmo {

class ObservableObject;
class Robot;
  
class DockingComponent : public IDependencyManagedComponent<RobotComponentID>, private Util::noncopyable
{
public:

  DockingComponent();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComps) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {};
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {};
  //////
  // end IDependencyManagedComponent functions
  //////
  
  // Tell the robot to docking with the specified object with markerCode using
  // dockAction.
  // Optionally takes
  // - offsets from the marker at which to dock
  // - how many firmware side retries should occur
  // - which docking method to use
  // - whether or not to a lift load check
  Result DockWithObject(const ObjectID objectID,
                        const f32 speed_mmps,
                        const f32 accel_mmps2,
                        const f32 decel_mmps2,
                        const Vision::KnownMarker::Code markerCode,
                        const Vision::KnownMarker::Code markerCode2,
                        const DockAction dockAction,
                        const f32 placementOffsetX_mm = 0,
                        const f32 placementOffsetY_mm = 0,
                        const f32 placementOffsetAngle_rad = 0,
                        const u8 numRetries = 2,
                        const DockingMethod dockingMethod = DockingMethod::BLIND_DOCKING,
                        const bool doLiftLoadCheck = false);
  
  // Tells the robot to abort docking
  Result AbortDocking() const;
  
  // Sends an updated docking error signal to the robot if we are currently docking
  void UpdateDockingErrorSignal(const RobotTimeStamp_t t) const;
  
  void SetPickingOrPlacing(bool t) {_isPickingOrPlacing = t;}
  bool IsPickingOrPlacing() const {return _isPickingOrPlacing;}
  
  void SetLastPickOrPlaceSucceeded(bool tf) { _lastPickOrPlaceSucceeded = tf;  }
  bool GetLastPickOrPlaceSucceeded() const { return _lastPickOrPlaceSucceeded; }
  
  const ObjectID& GetDockObject() const {return _dockObjectID;}
  const Vision::KnownMarker::Code& GetDockMarkerCode() const { return _dockMarkerCode;}
  void UnsetDockObjectID() { _dockObjectID.UnSet(); _dockMarkerCode = Vision::MARKER_INVALID; }
  
  // lets the robot decide if we should try to stack on top of the given object, so that we have a central place
  // to make the appropriate checks.
  // returns true if we should try to stack on top of the given object, false if something would prevent it,
  // for example if we think that block has something on top or it's too high to reach
  bool CanStackOnTopOfObject(const ObservableObject& object) const;
  
  // lets the robot decide if we should try to pick up the given object (assuming it is flat, not picking up
  // out of someone's hand). Checks that object is flat, not moving, no unknown pose, etc.
  bool CanPickUpObject(const ObservableObject& object) const;
  
  // same as above, but check that the block is on the ground (as opposed to stacked, on top of a notebook or
  // something, or in someone's hand
  bool CanPickUpObjectFromGround(const ObservableObject& object) const;

private:

  // Helper for CanStackOnTopOfObject and CanPickUpObjectFromGround
  bool CanInteractWithObjectHelper(const ObservableObject& object, Pose3d& relPose) const;
  
  Robot* _robot = nullptr;
  
  bool _isPickingOrPlacing    = false;
  bool _lastPickOrPlaceSucceeded = false;
  
  // We can't store pointers to makers because the object may be unobserved and reoverserved, which
  // could cause a located instance to be destroyed and recreated
  ObjectID                  _dockObjectID;
  Vision::KnownMarker::Code _dockMarkerCode = Vision::MARKER_INVALID;
  f32                       _dockPlacementOffsetX_mm = 0;
  f32                       _dockPlacementOffsetY_mm = 0;
  f32                       _dockPlacementOffsetAngle_rad = 0;

};

}
}



#endif
