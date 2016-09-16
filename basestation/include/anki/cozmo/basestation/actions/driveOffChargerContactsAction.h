/**
 * File: driveOffChargerContactsAction.h
 *
 * Author: Andrew Stein
 * Date:   9/13/2016
 *
 * Description: Defines an action to drive forward slightly, just enough to 
 *              get off the charger contacts, primarily for use in the SDK, since
 *              motors are locked while on charger to prevent hardware damage.
 *
 *              NOTE: does nothing if robot is not currently on charger.
 *
 *              NOTE: *intentionally* does not lock body track since this is to be used
 *                 when everything is likely locked by SDK mode being on charger
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Anki_Cozmo_DriveOffChargerContactsAction_H__
#define __Anki_Cozmo_DriveOffChargerContactsAction_H__

#include "anki/cozmo/basestation/actions/basicActions.h"

namespace Anki {
namespace Cozmo {
  
class DriveOffChargerContactsAction : public DriveStraightAction
{
public:
  
  DriveOffChargerContactsAction(Robot& robot);
  virtual ~DriveOffChargerContactsAction() { }
  
protected:
  
  virtual ActionResult Init() override;
  virtual ActionResult CheckIfDone() override;
  
private:
  
  bool  _startedOnCharger;
  
}; // class DriveOffChargerContactsAction
  
  
} // namespace Cozmo
} // namespace Anki

#endif /* __Anki_Cozmo_DriveOffChargerContactsAction_H__ */
