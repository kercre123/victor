/**
 * File: behaviorSelfTestLookAtCharger.h
 *
 * Author: Al Chaussee
 * Created: 11/16/2018
 *
 * Description: Turns towards a target marker and records a number of distance sensor readings
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorSelfTestLookAtCharger_H__
#define __Cozmo_Basestation_Behaviors_BehaviorSelfTestLookAtCharger_H__

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/iBehaviorSelfTest.h"

namespace Anki {
namespace Vector {

class BehaviorSelfTestLookAtCharger : public IBehaviorSelfTest
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorSelfTestLookAtCharger(const Json::Value& config);
  
protected:
  virtual void GetBehaviorOperationModifiersInternal(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOnCharger = true;
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
    modifiers.visionModesForActiveScope->insert({VisionMode::Markers, EVisionUpdateFrequency::High});
  }

  virtual Result         OnBehaviorActivatedInternal() override;
  virtual SelfTestStatus SelfTestUpdateInternal()      override;
  virtual void           OnBehaviorDeactivated()       override;

  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
private:

  // If seeing the expected marker/object, turns us to be perpendicular to it
  void TransitionToRefineTurn();

  // Starts recording distance sensor readings
  void TransitionToRecordSensor();

  // Turns back to the starting angle
  void TransitionToTurnBack();
  
  // Gets the most recently observed marker pose (wrt robot) of the expected object
  // Returns true if markerPoseWrtRobot is valid
  bool GetExpectedObjectMarkerPoseWrtRobot(Pose3d& markerPoseWrtRobot, ObjectID& objectID);
  
  // Initial starting angle when the behavior started
  Radians    _startingAngle               = 0;

  // The angle to turn to be able to see the expected object/marker
  Radians    _angleToTurn                 = 0;

  // The distance to drive forwards to see the target
  u32        _distToDrive_mm              = 0;

  // The object type we expect to see after turning "_angleToTurn"
  ObjectType _expectedObjectType          = ObjectType::UnknownObject;

  // The expected distance we should be seeing the expected object at
  f32        _expectedDistanceToObject_mm = 0;

  // The number of distance sensor readings left to record
  int        _numRecordedReadingsLeft     = -1;
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorSelfTestLookAtCharger_H__
