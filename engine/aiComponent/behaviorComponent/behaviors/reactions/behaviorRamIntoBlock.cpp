/**
 * File: behaviorRamIntoBlock.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2/21/17
 *
 * Description: Behavior to ram into a block
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorRamIntoBlock.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/dockActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"

#include "anki/common/basestation/math/pose.h"

namespace Anki {
namespace Cozmo {

namespace{
const float kDistancePastBlockToDrive_mm   = 100.0f;
const float kSpeedToDriveThroughBlock_mmps = 200.0f;
const float kDistanceBackUpFromBlock_mm    = 100.0f;
const float kSpeedBackUpFromBlock_mmps     = 100.0f;

} // end namespace
  
using namespace ExternalInterface;

BehaviorRamIntoBlock::BehaviorRamIntoBlock(const Json::Value& config)
: ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorRamIntoBlock::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return _targetID >= 0;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorRamIntoBlock::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(behaviorExternalInterface.GetRobotInfo().GetCarryingComponent().IsCarryingObject()){
    TransitionToPuttingDownBlock(behaviorExternalInterface);
  }else{
    TransitionToTurningToBlock(behaviorExternalInterface);
  }
  return Result::RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRamIntoBlock::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  _targetID = -1;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRamIntoBlock::TransitionToPuttingDownBlock(BehaviorExternalInterface& behaviorExternalInterface)
{
  auto& robotInfo = behaviorExternalInterface.GetRobotInfo();

  CompoundActionSequential* placeAction = new CompoundActionSequential();
  if(robotInfo.GetCarryingComponent().GetCarryingObject() != _targetID){
    const ObservableObject* obj = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(robotInfo.GetCarryingComponent().GetCarryingObject());
    if(obj != nullptr){
      Vec3f outVector;
      if(ComputeVectorBetween(robotInfo.GetPose(), obj->GetPose(), outVector)){
        Radians angle = FLT_NEAR(outVector.x(), 0.0f) ?
                  Radians(0)                          :
                  Radians(atanf(outVector.y()/outVector.x()));
        const int offsetDirection = angle.ToFloat() > 0 ? 1 : -1;
        angle += DEG_TO_RAD(90 * offsetDirection);
        const bool isAbsolute = true;
        
        // Overshoot the angle to ram by a quarter turn and then place the block down
        placeAction->AddAction(new TurnInPlaceAction(angle.ToFloat(), isAbsolute));
      }
    }
  }
  
  placeAction->AddAction(new PlaceObjectOnGroundAction());
  DelegateIfInControl(placeAction, &BehaviorRamIntoBlock::TransitionToTurningToBlock);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRamIntoBlock::TransitionToTurningToBlock(BehaviorExternalInterface& behaviorExternalInterface)
{
  CompoundActionParallel* action = new CompoundActionParallel({
    new TurnTowardsObjectAction(_targetID),
    new MoveLiftToHeightAction( MoveLiftToHeightAction::Preset::CARRY)
  });
  DelegateIfInControl(action, &BehaviorRamIntoBlock::TransitionToRammingIntoBlock);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRamIntoBlock::TransitionToRammingIntoBlock(BehaviorExternalInterface& behaviorExternalInterface)
{  
  const ObservableObject* obj = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_targetID);
  if(obj != nullptr){
    auto& robotPose = behaviorExternalInterface.GetRobotInfo().GetPose();

    const f32 distToObj = ComputeDistanceBetween(robotPose, obj->GetPose());
    
    
    CompoundActionParallel* ramAction = new CompoundActionParallel({
      new DriveStraightAction(
                              distToObj + kDistancePastBlockToDrive_mm,
                              kSpeedToDriveThroughBlock_mmps,
                              false),
      new TriggerAnimationAction(AnimationTrigger::SoundOnlyRamIntoBlock)
    });
    

    IActionRunner* driveOut = new DriveStraightAction(
                                                      -kDistanceBackUpFromBlock_mm,
                                                      kSpeedBackUpFromBlock_mmps);
    
    CompoundActionSequential* driveAction = new CompoundActionSequential();
    driveAction->AddAction(ramAction);
    driveAction->AddAction(driveOut);
    DelegateIfInControl(driveAction);
  }
}
  
} // namespace Cozmo
} // namespace Anki
