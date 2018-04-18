/**
 * File: chargerActions.h
 *
 * Author: Matt Michini
 * Date:   8/10/2017
 *
 * Description: Implements charger-related actions, e.g. docking with the charger.
 *
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef __Anki_Cozmo_Engine_ChargerActions_H__
#define __Anki_Cozmo_Engine_ChargerActions_H__

#include "engine/actions/actionInterface.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/dockActions.h"
#include "engine/actionableObject.h"

namespace Anki {
namespace Cozmo {
    
// Forward Declarations:
class Robot;

// MountChargerAction
//
// Drive backward onto the charger, optionally using the cliff sensors to
// detect the charger docking pattern and correct while reversing.
class MountChargerAction : public IAction
{
public:
  MountChargerAction(ObjectID chargerID,
                     const bool useCliffSensorCorrection = true,
                     const bool shouldPlayDrivingAnimation = true);
  ~MountChargerAction();
  
protected:
  
  virtual ActionResult Init() override;
  virtual ActionResult CheckIfDone() override;
    
private:
  const ObjectID _chargerID;

  const bool _useCliffSensorCorrection;
  const bool _playDrivingAnimation;
  
  // Pointers to compound actions which comprise this action:
  std::unique_ptr<ICompoundAction> _mountAction = nullptr;
  std::unique_ptr<DriveStraightAction> _driveForRetryAction = nullptr;
  
  // Allocate and add actions to the member compound actions:
  ActionResult ConfigureMountAction();
  ActionResult ConfigureDriveForRetryAction();
  
}; // class MountChargerAction


  
// TurnToAlignWithChargerAction
//
// Compute the proper angle to turn, and turn away from
// the charger to prepare for backing up onto it.
// Optionally play an animation depending on turn direction.
class TurnToAlignWithChargerAction : public IAction
{
public:
  TurnToAlignWithChargerAction(ObjectID chargerID,
                               AnimationTrigger leftTurnAnimTrigger = AnimationTrigger::Count,
                               AnimationTrigger rightTurnAnimTrigger = AnimationTrigger::Count);
  
protected:
  virtual void GetRequiredVisionModes(std::set<VisionModeRequest>& requests) const override;
  virtual ActionResult Init() override;
  virtual ActionResult CheckIfDone() override;
  
private:
  const ObjectID _chargerID;
  
  const AnimationTrigger _leftTurnAnimTrigger;
  const AnimationTrigger _rightTurnAnimTrigger;
  
  std::unique_ptr<CompoundActionParallel> _compoundAction = nullptr;
  
}; // class TurnToAlignWithChargerAction
  
  
// BackupOntoChargerAction
//
// Reverse onto the charger, stopping when charger contacts are sensed.
// Optionally, use the cliff sensors to correct heading while reversing.
class BackupOntoChargerAction : public IDockAction
{
public:
  BackupOntoChargerAction(ObjectID chargerID,
                          bool useCliffSensorCorrection);
  
protected:

  virtual ActionResult InitInternal() override;
  
  virtual ActionResult SelectDockAction(ActionableObject* object) override;
  
  virtual PreActionPose::ActionType GetPreActionType() override { return PreActionPose::DOCKING; }
  
  //virtual f32 GetTimeoutInSeconds() const override { return 5.f; }
  
  // Add a slight delay before verification to allow the "isOnCharger"
  // bit to turn on (it has a slight debounce)
  virtual f32 GetVerifyDelayInSeconds() const override { return 0.25f; }
  
  virtual ActionResult Verify() override;
  
private:
  
  // If true, use the cliff sensors to detect the light/dark pattern
  // while reversing onto the charger and adjust accordingly
  const bool _useCliffSensorCorrection;
  
  // Pitch angle just before starting the backup action
  Radians _initialPitchAngle = 0.f;
  
}; // class BackupOntoChargerAction


// DriveToAndMountChargerAction
//
// Drive to the charger and mount it.
class DriveToAndMountChargerAction : public CompoundActionSequential
{
public:
  DriveToAndMountChargerAction(const ObjectID& objectID,
                               const bool useCliffSensorCorrection = true);
  
  virtual ~DriveToAndMountChargerAction() { }
}; // class DriveToAndMountChargerAction

  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_Engine_ChargerActions_H__
