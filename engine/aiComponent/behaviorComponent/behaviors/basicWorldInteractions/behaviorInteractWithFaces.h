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

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/events/animationTriggerHelpers.h"
#include "engine/smartFaceId.h"
#include "coretech/vision/engine/faceIdTypes.h"

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

class BehaviorInteractWithFaces : public ICozmoBehavior
{
protected:
    
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorInteractWithFaces(const Json::Value& config);
    
public:

  virtual bool WantsToBeActivatedBehavior() const override;
    
protected:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ICozmoBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.visionModesForActivatableScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::Low });
    modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::Standard });
  }

  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ICozmoBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

private:
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    
  using BaseClass = ICozmoBehavior;
    
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  using Face = Vision::TrackedFace;

  void LoadConfig(const Json::Value& config);

  // sets the mutbale _targetFace to the face we want to interact with
  void SelectFaceToTrack() const;

  void TransitionToInitialReaction();
  void TransitionToGlancingDown();
  void TransitionToDrivingForward();
  void TransitionToTrackingFace();
  void TransitionToTriggerEmotionEvent();

  bool CanDriveIdealDistanceForward();

  ////////////////////////////////////////////////////////////////////////////////
  // Members
  ////////////////////////////////////////////////////////////////////////////////

  // ID of face we are currently interested in
  mutable SmartFaceID _targetFace;

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
