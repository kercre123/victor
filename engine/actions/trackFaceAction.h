/**
 * File: trackFaceAction.h
 *
 * Author: Andrew Stein
 * Date:   12/11/2015
 *
 * Description: Defines an action for tracking (human) faces, derived from ITrackAction
 *
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef __Anki_Cozmo_Basestation_TrackFaceAction_H__
#define __Anki_Cozmo_Basestation_TrackFaceAction_H__

#include "anki/vision/basestation/trackedFace.h"
#include "engine/actions/trackActionInterface.h"
#include "engine/smartFaceId.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Cozmo {

class TrackFaceAction : public ITrackAction
{
public:
  
  using FaceID = Vision::FaceID_t;
  
  TrackFaceAction(Robot& robot, FaceID faceID);
  TrackFaceAction(Robot& robot, SmartFaceID faceID);
  virtual ~TrackFaceAction();

  virtual void GetCompletionUnion(ActionCompletedUnion& completionInfo) const override;
  
protected:
  
  virtual ActionResult InitInternal() override;
  
  // Required by ITrackAction:
  virtual UpdateResult UpdateTracking(Radians& absPanAngle, Radians& absTiltAngle, f32& distance_mm) override;
  
private:

  SmartFaceID          _faceID;
  TimeStamp_t          _lastFaceUpdate = 0;

  Signal::SmartHandle _signalHandle;

}; // class TrackFaceAction
    
} // namespace Cozmo
} // namespace Anki

#endif /* __Anki_Cozmo_Basestation_TrackFaceAction_H__ */
