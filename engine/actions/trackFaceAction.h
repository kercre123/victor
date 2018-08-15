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

#include "coretech/vision/engine/trackedFace.h"
#include "engine/actions/trackActionInterface.h"
#include "engine/smartFaceId.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Cozmo {

class TrackFaceAction : public ITrackAction
{
public:
  
  using FaceID = Vision::FaceID_t;
  
  explicit TrackFaceAction(FaceID faceID);
  explicit TrackFaceAction(SmartFaceID faceID);
  virtual ~TrackFaceAction();

  virtual void GetCompletionUnion(ActionCompletedUnion& completionInfo) const override;
  void SetStopCriteriaWithEyeContactOverride(const f32 duration_sec, const f32 noEyeContactTimeout_sec,
                                             const TimeStamp_t eyeContactWithinLast_ms);
  
protected:
  
  virtual void GetRequiredVisionModes(std::set<VisionModeRequest>& requests) const override;

  virtual ActionResult InitInternal() override;
  
  // Required by ITrackAction:
  virtual UpdateResult UpdateTracking(Radians& absPanAngle, Radians& absTiltAngle, f32& distance_mm) override;
  
  virtual void OnRobotSet() override final;

private:
  virtual bool ContinueCriteriaMet(const f32 currentTime_sec) override;
  struct {
    Radians panTol                      = -1.f;
    Radians tiltTol                     = -1.f;
    f32     minDist_mm                  = -1.f;
    f32     maxDist_mm                  = -1.f;
    f32     duration_sec                = 0.f; // _stopCriteria is ignored if this is 0
    f32     withinTolSince_sec          = 0.f;
    bool    interruptDrivingAnim        = false;
    f32     noEyeContactTimeout_sec     = 0.f;
    f32     timeOfLastEyeContact_sec    = 0.f;
    TimeStamp_t eyeContactWithinLast_ms = 0;
  } _stopCriteria;

  // store face id as non-smart until robot is accessible
  FaceID               _tmpFaceID;
  
  SmartFaceID          _faceID;
  TimeStamp_t          _lastFaceUpdate = 0;

}; // class TrackFaceAction
    
} // namespace Cozmo
} // namespace Anki

#endif /* __Anki_Cozmo_Basestation_TrackFaceAction_H__ */
