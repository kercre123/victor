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

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/smartFaceId.h"

namespace Anki {
namespace Cozmo {

class Robot;

class BehaviorDriveToFace : public IBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorDriveToFace(Robot& robot, const Json::Value& config);

  virtual Result InitInternal(Robot& robot) override;
  virtual IBehavior::Status UpdateInternal(Robot& robot) override;
  
  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}

private:
  using base = IBehavior;
  enum class State {
    TurnTowardsFace,
    DriveToFace,
    TrackFace
  };
  
  State _currentState;
  float _timeCancelTracking_s;
  SmartFaceID _targetFace;
  
  void TransitionToTurningTowardsFace(Robot& robot);
  void TransitionToDrivingToFace(Robot& robot);
  void TransitionToTrackingFace(Robot& robot);
  
  void SetState_internal(State state, const std::string& stateName);

  
};


}
}


#endif
