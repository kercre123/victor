/**
 * File: behaviorPlaypenDistanceSensor.h
 *
 * Author: Al Chaussee
 * Created: 09/22/17
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlaypenDistanceSensor_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlaypenDistanceSensor_H__

#include "engine/behaviorSystem/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

namespace Anki {
namespace Cozmo {

class BehaviorPlaypenDistanceSensor : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPlaypenDistanceSensor(Robot& robot, const Json::Value& config);
  
protected:
  
  virtual Result         InternalInitInternal(Robot& robot)   override;
  virtual BehaviorStatus InternalUpdateInternal(Robot& robot) override;
  virtual void           StopInternal(Robot& robot)           override;
  
  virtual bool ShouldRunWhileOnCharger() const override { return true; }
  virtual bool CarryingObjectHandledInternally() const override { return true; }
  
private:

  void TransitionToRefineTurn(Robot& robot);
  void TransitionToRecordSensor(Robot& robot);
  void TransitionToTurnBack(Robot& robot);
  
  Radians    _startingAngle = 0;
  Radians    _angleToTurn = 0;
  ObjectType _expectedObjectType = ObjectType::UnknownObject;
  int        _numRecordedReadingsLeft = -1;
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenDistanceSensor_H__
