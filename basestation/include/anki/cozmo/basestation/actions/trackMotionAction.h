/**
 * File: trackMotionAction.h
 *
 * Author: Andrew Stein
 * Date:   12/11/2015
 *
 * Description: Defines an action for tracking motion (on the ground), derived from ITrackAction.
 *
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef __Anki_Cozmo_Basestation_TrackMotionAction_H__
#define __Anki_Cozmo_Basestation_TrackMotionAction_H__

#include "anki/cozmo/basestation/actions/trackActionInterface.h"

#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {

class TrackMotionAction : public ITrackAction
{
public:
  
  TrackMotionAction(Robot& robot) : ITrackAction(robot, "TrackMotion", RobotActionType::TRACK_MOTION) { }
  
protected:
  
  virtual ActionResult InitInternal() override;
  
  // Required by ITrackAction:
  virtual UpdateResult UpdateTracking(Radians& absPanAngle, Radians& absTiltAngle, f32& distance_mm) override;
  
private:
  
  bool _gotNewMotionObservation = false;
  
  ExternalInterface::RobotObservedMotion _motionObservation;
  
  Signal::SmartHandle _signalHandle;
  
}; // class TrackMotionAction
    
} // namespace Cozmo
} // namespace Anki

#endif /* __Anki_Cozmo_Basestation_TrackMotionAction_H__ */
