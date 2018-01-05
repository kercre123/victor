/**
 * File: behaviorRndAudioDemo.cpp
 *
 * Author: Matt Michini
 * Created: 1/4/18
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorRndAudioDemo.h"

#include "engine/actions/basicActions.h"
#include "engine/actions/chargerActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"

namespace Anki {
namespace Cozmo {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorRndAudioDemo::BehaviorRndAudioDemo(const Json::Value& config)
  : ICozmoBehavior(config)
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRndAudioDemo::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  PRINT_NAMED_WARNING("BehaviorRndAudioDemo", "starting behavior");

  // Move head up and wait until we see all cubes:
  auto action = new CompoundActionSequential();
  
  action->AddAction(new MoveHeadToAngleAction(0.f));
  
  // wait until we have 3 located cubes:
  auto waitForCubesLambda = [&behaviorExternalInterface](Robot& robot) {
    BlockWorldFilter filter;
    filter.AddAllowedFamily(ObjectFamily::LightCube);
    
    std::vector<const ObservableObject*> locatedBlocks;
    behaviorExternalInterface.GetBlockWorld().FindLocatedMatchingObjects(filter, locatedBlocks);
    
    // Ensure there are three located blocks:
    return (locatedBlocks.size() == 3);
  };
  action->AddAction(new WaitForLambdaAction(waitForCubesLambda));
  
  DelegateIfInControl(action, &BehaviorRndAudioDemo::StartSequence);
}

void BehaviorRndAudioDemo::StartSequence(BehaviorExternalInterface& behaviorExternalInterface)
{
  PRINT_NAMED_WARNING("BehaviorRndAudioDemo", "starting sequence");
 
  // Sequence (for each cube):
  // 1. turn toward a cube
  // 2. drive up to it
  // 3. reverse back from it

  // Queue up the sequence for each object
  BlockWorldFilter filter;
  filter.AddAllowedFamily(ObjectFamily::LightCube);
  std::vector<const ObservableObject*> locatedBlocks;
  behaviorExternalInterface.GetBlockWorld().FindLocatedMatchingObjects(filter, locatedBlocks);
  
  auto compoundAction = new CompoundActionSequential();
  compoundAction->SetDelayBetweenActions(0.5f);
  for (const auto& obj : locatedBlocks) {
    compoundAction->AddAction(new TurnTowardsObjectAction(obj->GetID()));
    
    float currentDistanceToCube_mm = 0.f;
    ComputeDistanceBetween(behaviorExternalInterface.GetRobotInfo().GetPose(), obj->GetPose(), currentDistanceToCube_mm);
    
    const float distanceAtWhichToStop_mm = 80.f;
    const float drivingDistance_mm = currentDistanceToCube_mm - distanceAtWhichToStop_mm;
    const float drivingSpeed_mmps = 75.f;
    
    // Drive up to the object..
    compoundAction->AddAction(new DriveStraightAction(drivingDistance_mm, drivingSpeed_mmps, false));
    
    // Drive back from the object..
    compoundAction->AddAction(new DriveStraightAction(-drivingDistance_mm, drivingSpeed_mmps, false));
  }
  
  DelegateIfInControl(compoundAction, &BehaviorRndAudioDemo::OnBehaviorActivated);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRndAudioDemo::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{

}

  
} // namespace Cozmo
} // namespace Anki
