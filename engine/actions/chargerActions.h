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
                     const bool useCliffSensorCorrection = true);
  
protected:
  
  virtual ActionResult Init() override;
  virtual ActionResult CheckIfDone() override;
    
private:
  const ObjectID _chargerID;

  const bool _useCliffSensorCorrection;
  
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
class TurnToAlignWithChargerAction : public TurnInPlaceAction
{
using BaseClass = TurnInPlaceAction;
public:
  TurnToAlignWithChargerAction(ObjectID chargerID);
  
protected:
  virtual ActionResult Init() override;
  
private:
  const ObjectID _chargerID;
  
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

  virtual ActionResult SelectDockAction(ActionableObject* object) override;
  
  virtual PreActionPose::ActionType GetPreActionType() override { return PreActionPose::DOCKING; }
  
  //virtual f32 GetTimeoutInSeconds() const override { return 5.f; }
  
  virtual ActionResult Verify() override;
  
private:
  
  // If true, use the cliff sensors to detect the light/dark pattern
  // while reversing onto the charger and adjust accordingly
  const bool _useCliffSensorCorrection;
  
}; // class BackupOntoChargerAction


// DriveToAndMountChargerAction
//
// Drive to the charger and mount it.
class DriveToAndMountChargerAction : public CompoundActionSequential
{
public:
  DriveToAndMountChargerAction(const ObjectID& objectID,
                               const bool useCliffSensorCorrection = true,
                               const bool useManualSpeed = false);
  
  virtual ~DriveToAndMountChargerAction() { }
}; // class DriveToAndMountChargerAction

  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_Engine_ChargerActions_H__
