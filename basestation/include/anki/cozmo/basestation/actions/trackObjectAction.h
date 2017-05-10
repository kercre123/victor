/**
 * File: trackObjectAction.h
 *
 * Author: Andrew Stein
 * Date:   12/11/2015
 *
 * Description: Defines an action for tracking objects (from BlockWorld), derived from ITrackAction
 *
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef __Anki_Cozmo_Basestation_TrackObjectAction_H__
#define __Anki_Cozmo_Basestation_TrackObjectAction_H__

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/basestation/actions/trackActionInterface.h"

namespace Anki {
namespace Cozmo {

class TrackObjectAction : public ITrackAction
{
public:
  TrackObjectAction(Robot& robot, const ObjectID& objectID, bool trackByType = true);
  virtual ~TrackObjectAction();

protected:
  
  virtual ActionResult InitInternal() override;
  
  // Required by ITrackAction:
  virtual UpdateResult UpdateTracking(Radians& absPanAngle, Radians& absTiltAngle, f32& distance_mm) override;
  
private:
  
  ObjectID     _objectID;
  ObjectType   _objectType;
  bool         _trackByType;
  Pose3d       _lastTrackToPose;
  
}; // class TrackObjectAction

    
} // namespace Cozmo
} // namespace Anki

#endif /* __Anki_Cozmo_Basestation_TrackObjectAction_H__ */
