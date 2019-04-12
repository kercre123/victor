/**
 * File: localizationComponent.h
 *
 * Author: Michael Willett
 * Created: 3/21/2019
 *
 * Description: Maintains ownership of main localization related tasks
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_Components_LocalizationComponent_H__
#define __Engine_Components_LocalizationComponent_H__

#include "engine/robot.h"

#include "coretech/common/engine/objectIDs.h"
#include "coretech/common/engine/robotTimeStamp.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "engine/robotComponents_fwd.h"


#include "coretech/common/engine/math/poseOriginList.h"

#include "util/helpers/noncopyable.h"

namespace Anki {
namespace Vector {

class ObservableObject;
  
class LocalizationComponent : public IDependencyManagedComponent<RobotComponentID>, private Util::noncopyable
{
public:

  LocalizationComponent();
  ~LocalizationComponent();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {};
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {};
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  //////
  // end IDependencyManagedComponent functions
  //////  

  bool IsLocalized() const;
  inline bool HasMovedSinceBeingLocalized() const { return _hasMovedSinceLocalization; }
  inline void SetRobotHasMoved(bool isMoving) { _hasMovedSinceLocalization |= isMoving; }


  // Get the ID of the object we are localized to
  const ObjectID& GetLocalizedTo() const {return _localizedToID;}
  const PoseFrameID_t GetPoseFrameID()  const { return _frameId; }
  const Pose3d& GetWorldOrigin()  const { return _poseOrigins.GetCurrentOrigin(); }
  const Pose3d& GetDriveCenterPose(void) const;
  const PoseOriginList& GetPoseOriginList() const { return _poseOrigins; }

  // Set the object we are localized to.
  // Use nullptr to UnSet the localizedTo object but still mark the robot
  // as localized (i.e. to "odometry").
  Result SetLocalizedTo(const ObservableObject* object);

  Result LocalizeToObject(const ObservableObject* seenObject, ObservableObject* existingObject);

  void Delocalize(bool isCarryingObject);


  // Updates the pose of the robot.
  // Sends new pose down to robot (on next tick).
  Result SetNewPose(const Pose3d& newPose);

  // Updates pose to be on charger
  Result SetPoseOnCharger();

  // Update's the robot's pose to be in front of the
  // charger as if it had just rolled off the charger.
  Result SetPosePostRollOffCharger();


  // Sets the charger that it's docking to
  void           SetCharger(const ObjectID& chargerID) { _chargerID = chargerID; }
  const ObjectID GetCharger() const                    { return _chargerID; }

  // Updates the current pose to the best estimate based on
  // historical poses including vision-based poses.
  // Returns true if the pose is successfully updated, false otherwise.
  bool UpdateCurrPoseFromHistory();

  // TODO: make private
  Result SendAbsLocalizationUpdate(const Pose3d&             pose,
                                   const RobotTimeStamp_t&   t,
                                   const PoseFrameID_t&      frameId) const;


  Result NotifyOfRobotState(const RobotState& msg);

private:
  // Sets robot pose but does not update the pose on the robot.
  // Unless you know what you're doing you probably want to use
  // the public function SetNewPose()
  void SetPose(const Pose3d &newPose);


  // Increments frameID and adds a vision-only pose to history
  // Sets a flag to send a localization update on the next tick
  Result AddVisionOnlyStateToHistory(const RobotTimeStamp_t t,
                                     const Pose3d& pose,
                                     const f32 head_angle,
                                     const f32 lift_angle);
  // Send robot's current pose
  Result SendAbsLocalizationUpdate() const;

  Robot*         _robot = nullptr;
  PoseOriginList _poseOrigins;

  Pose3d         _driveCenterPose;
  PoseFrameID_t  _frameId               = 0;
  u32            _numMismatchedFrameIDs = 0;


  bool     _isLocalized                  = true;
  bool     _hasMovedSinceLocalization    = false;
  bool     _localizedToFixedObject;                  // false until robot sees a _fixed_ mat
  ObjectID _localizedToID;                           // ID of mat object robot is localized to
  ObjectID _chargerID;                                // Charge base ID that is being docked to
  bool     _needToSendLocalizationUpdate = false;


  // f32      _localizedMarkerDistToCameraSq = -1.0f;   // Stores (squared) distance to the closest observed marker of the object we're localized to
};

inline const Pose3d& LocalizationComponent::GetDriveCenterPose(void) const
{
  // TODO: COZMO-1637: Once we figure this out, switch this back to dev_assert for efficiency
  ANKI_VERIFY(_driveCenterPose.HasSameRootAs(GetWorldOrigin()),
              "Robot.GetDriveCenterPose.PoseOriginNotWorldOrigin",
              "WorldOrigin: %s, Pose: %s",
              GetWorldOrigin().GetNamedPathToRoot(false).c_str(),
              _driveCenterPose.GetNamedPathToRoot(false).c_str());

  return _driveCenterPose;
}

inline bool LocalizationComponent::IsLocalized() const {

  DEV_ASSERT(_isLocalized || (!_isLocalized && !_localizedToID.IsSet()),
             "Robot can't think it is localized and have localizedToID set!");

  return _isLocalized;
}

}
}



#endif
