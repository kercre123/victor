/**
 * File: behaviorInteractWithFaces.h
 *
 * Author: Brad Neuman
 * Created: 2016-08-22
 *
 * Description: Turn towards a face and "interact" with it, which for now just means drive towards it and
 *              track it for a while.
 *
 * NOTE: This behavior shares a name with an older "interact with faces" behavior, but has been re-written
 * since then
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorInteractWithFaces_H__
#define __Cozmo_Basestation_Behaviors_BehaviorInteractWithFaces_H__

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/vision/basestation/faceIdTypes.h"

#include <string>
#include <unordered_map>

namespace Anki {

namespace Vision {
class TrackedFace;
}

namespace Cozmo {

namespace ExternalInterface {
struct RobotObservedFace;
struct RobotDeletedFace;
struct RobotChangedObservedFaceID;
}

class BehaviorInteractWithFaces : public IBehavior
{
protected:
    
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorInteractWithFaces(Robot& robot, const Json::Value& config);
    
public:

  virtual bool CarryingObjectHandledInternally() const override { return false;}
  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const override;
    
protected:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    
  virtual Result InitInternal(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;
  virtual void   AlwaysHandle(const EngineToGameEvent& event, const Robot& robot) override;

private:
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    
  using BaseClass = IBehavior;
    
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  using Face = Vision::TrackedFace;
  using FaceID_t = Vision::FaceID_t;

  void LoadConfig(const Json::Value& config);

  // sets the mutbale _targetFace to the face we want to interact with
  void SelectFaceToTrack(const Robot& robot) const;

  void TransitionToInitialReaction(Robot& robot);
  void TransitionToGlancingDown(Robot& robot);
  void TransitionToDrivingForward(Robot& robot);
  void TransitionToTrackingFace(Robot& robot);
  void TransitionToTriggerEmotionEvent(Robot& robot);

  bool CanDriveIdealDistanceForward(const Robot& robot);

  ////////////////////////////////////////////////////////////////////////////////
  // Members
  ////////////////////////////////////////////////////////////////////////////////

  // ID of face we are currently interested in
  mutable FaceID_t _targetFace = Vision::UnknownFaceID;

  // We only want to run for faces we've seen since the last time we ran, so keep track of the final timestamp
  // when the behavior finishes
  TimeStamp_t _lastImageTimestampWhileRunning = 0;

  // In the face tracking stage the action will hang, so store a time at which we want to stop it (from within
  // Update)
  float _trackFaceUntilTime_s = -1.0f;

  struct Configuration {
    float minTimeToTrackFace_s = 0.0f;;
    float maxTimeToTrackFace_s = 0.0f;;

    float minClampPeriod_s = 0.0f;;
    float maxClampPeriod_s = 0.0f;;
    bool clampSmallAngles = false;
  };

  Configuration _configParams;

    
}; // BehaviorInteractWithFaces
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorInteractWithFaces_H__
