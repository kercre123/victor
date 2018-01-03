/**
 * File: behaviorPlaypenDistanceSensor.h
 *
 * Author: Al Chaussee
 * Created: 09/22/17
 *
 * Description: Turns towards a target marker and records a number of distance sensor readings
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlaypenDistanceSensor_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlaypenDistanceSensor_H__

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

namespace Anki {
namespace Cozmo {

class BehaviorPlaypenDistanceSensor : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPlaypenDistanceSensor(const Json::Value& config);
  
protected:
  
  virtual Result         OnBehaviorActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface)   override;
  virtual PlaypenStatus PlaypenUpdateInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void           OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)           override;
  
  virtual bool ShouldRunWhileOnCharger() const override { return true; }
  virtual bool CarryingObjectHandledInternally() const override { return true; }
  
private:

  // If seeing the expected marker/object, turns us to be perpendicular to it
  void TransitionToRefineTurn(BehaviorExternalInterface& behaviorExternalInterface);

  // Starts recording distance sensor readings
  void TransitionToRecordSensor(BehaviorExternalInterface& behaviorExternalInterface);

  // Turns back to the starting angle
  void TransitionToTurnBack(BehaviorExternalInterface& behaviorExternalInterface);
  
  // Gets the most recently observed marker pose (wrt robot) of the expected object
  // Returns true if markerPoseWrtRobot is valid
  bool GetExpectedObjectMarkerPoseWrtRobot(BehaviorExternalInterface& behaviorExternalInterface,
                                           Pose3d& markerPoseWrtRobot);
  
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

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenDistanceSensor_H__
