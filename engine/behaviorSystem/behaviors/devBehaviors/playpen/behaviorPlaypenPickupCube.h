/**
 * File: behaviorPlaypenPickupCube.h
 *
 * Author: Al Chaussee
 * Created: 08/08/17
 *
 * Description: Checks that Cozmo can pickup and place LightCube1 (paperclip) with minimal changes in body rotation
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlaypenPickupCube_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlaypenPickupCube_H__

#include "engine/behaviorSystem/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

namespace Anki {
namespace Cozmo {

class BehaviorPlaypenPickupCube : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPlaypenPickupCube(Robot& robot, const Json::Value& config);
  
protected:
  
  virtual Result         InternalInitInternal(Robot& robot)   override;
  virtual void           StopInternal(Robot& robot)   override;
  
  virtual void HandleWhileRunningInternal(const EngineToGameEvent& event, Robot& robot) override;
  
private:
  
  void TransitionToPickupCube(Robot& robot);
  void TransitionToPlaceCube(Robot& robot);
  void TransitionToBackup(Robot& robot);
  
  Pose3d _expectedCubePose;
  Radians _robotAngleAtPickup;
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenPickupCube_H__
