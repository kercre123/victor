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
namespace Cozmo {

class BehaviorPlaypenPickupCube : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPlaypenPickupCube(const Json::Value& config);
  
protected:
  
  virtual Result         OnBehaviorActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface)   override;
  virtual void           OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)   override;
  
  virtual void HandleWhileActivatedInternal(const RobotToEngineEvent& event, 
                                            BehaviorExternalInterface& behaviorExternalInterface) override;

private:
  
  void TransitionToWaitForCube(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToPickupCube(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToPlaceCube(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToBackup(BehaviorExternalInterface& behaviorExternalInterface);
  
  Pose3d _expectedCubePose;
  Radians _robotAngleAtPickup;
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenPickupCube_H__
