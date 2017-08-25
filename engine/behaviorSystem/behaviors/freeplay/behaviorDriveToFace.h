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

#include "engine/behaviorSystem/behaviors/iBehavior.h"
#include "engine/smartFaceId.h"

namespace Anki {
namespace Cozmo {

class Robot;

class BehaviorDriveToFace : public IBehavior
{
public:
  // Returns true if Cozmo is close enough to the face that he won't actually
  // drive forward when this behavior runs
  bool IsCozmoAlreadyCloseEnoughToFace(Robot& robot, Vision::FaceID_t faceID);
  
  void SetTargetFace(const SmartFaceID faceID){_targetFace = faceID;}
  
protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorDriveToFace(Robot& robot, const Json::Value& config);

  virtual Result InitInternal(Robot& robot) override;
  virtual IBehavior::Status UpdateInternal(Robot& robot) override;
  virtual void StopInternal(Robot& robot) override;

  
  virtual bool IsRunnableInternal(const Robot& robot) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}
  
private:
  using base = IBehavior;
  enum class State {
    TurnTowardsFace,
    DriveToFace,
    AlreadyCloseEnough,
    TrackFace
  };
  
  State _currentState;
  float _timeCancelTracking_s;
  mutable SmartFaceID _targetFace;
  
  void TransitionToTurningTowardsFace(Robot& robot);
  void TransitionToDrivingToFace(Robot& robot);
  void TransitionToAlreadyCloseEnough(Robot& robot);
  void TransitionToTrackingFace(Robot& robot);
  
  // Returns true if able to calculate distance to face - false otherwise
  bool CalculateDistanceToFace(Robot& robot, Vision::FaceID_t faceID, float& distance);
  void SetState_internal(State state, const std::string& stateName);

  
};


}
}


#endif
