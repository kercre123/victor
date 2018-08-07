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

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

namespace Anki {
namespace Vector {

class BehaviorPlaypenPickupCube : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPlaypenPickupCube(const Json::Value& config);
  
protected:
  
  virtual Result         OnBehaviorActivatedInternal()   override;
  virtual void           OnBehaviorDeactivated()   override;
  
  virtual void HandleWhileActivatedInternal(const RobotToEngineEvent& event) override;

private:
  
  void TransitionToWaitForCube();
  void TransitionToPickupCube();
  void TransitionToPlaceCube();
  void TransitionToBackup();
  
  Pose3d _expectedCubePose;
  Radians _robotAngleAtPickup;
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenPickupCube_H__
