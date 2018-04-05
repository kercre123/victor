/**
* File: behaviorDriveToFace.h
*
* Author: Kevin M. Karol
* Created: 2017-06-05
*
* Description: Behavior that turns towards and then drives to a face
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorDriveToFace_H__
#define __Cozmo_Basestation_Behaviors_BehaviorDriveToFace_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/smartFaceId.h"

namespace Anki {
namespace Cozmo {

class BehaviorDriveToFace : public ICozmoBehavior
{
public:
  // Returns true if Cozmo is close enough to the face that he won't actually
  // drive forward when this behavior runs
  bool IsCozmoAlreadyCloseEnoughToFace(Vision::FaceID_t faceID);
  
  void SetTargetFace(const SmartFaceID faceID){_targetFace = faceID;}
  
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorDriveToFace(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {}

  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  
private:
  using base = ICozmoBehavior;
  enum class State {
    TurnTowardsFace,
    DriveToFace,
    AlreadyCloseEnough,
    TrackFace
  };
  
  State _currentState;
  float _timeCancelTracking_s;
  mutable SmartFaceID _targetFace;
  
  void TransitionToTurningTowardsFace();
  void TransitionToDrivingToFace();
  void TransitionToAlreadyCloseEnough();
  void TransitionToTrackingFace();
  
  // Returns true if able to calculate distance to face - false otherwise
  bool CalculateDistanceToFace(Vision::FaceID_t faceID, float& distance);
  void SetState_internal(State state, const std::string& stateName);

  
};


}
}


#endif
