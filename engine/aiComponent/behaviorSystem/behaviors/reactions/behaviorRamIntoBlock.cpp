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

#include "engine/aiComponent/behaviorSystem/behaviors/reactions/behaviorRamIntoBlock.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/dockActions.h"
#include "engine/aiComponent/behaviorSystem/reactionTriggerStrategies/reactionTriggerHelpers.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"
#include "engine/robot.h"

#include "anki/common/basestation/math/pose.h"

namespace Anki {
namespace Cozmo {

namespace{
const float kDistancePastBlockToDrive_mm   = 100.0f;
const float kSpeedToDriveThroughBlock_mmps = 200.0f;
const float kDistanceBackUpFromBlock_mm    = 100.0f;
const float kSpeedBackUpFromBlock_mmps     = 100.0f;

  
constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersRamIntoBlockArray = {
  {ReactionTrigger::CliffDetected,                true},
  {ReactionTrigger::CubeMoved,                    false},
  {ReactionTrigger::FacePositionUpdated,          true},
  {ReactionTrigger::FistBump,                     false},
  {ReactionTrigger::Frustration,                  false},
  {ReactionTrigger::Hiccup,                       false},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        true},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          false},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::UnexpectedMovement,           true},
  {ReactionTrigger::VC,                           false}
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersRamIntoBlockArray),
              "Reaction triggers duplicate or non-sequential");

} // end namespace
  
using namespace ExternalInterface;

BehaviorRamIntoBlock::BehaviorRamIntoBlock(const Json::Value& config)
: IBehavior(config)
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
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();

  if(robot.GetCarryingComponent().IsCarryingObject()){
    TransitionToPuttingDownBlock(behaviorExternalInterface);
  }else{
    TransitionToTurningToBlock(behaviorExternalInterface);
  }
  return Result::RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorRamIntoBlock::ResumeInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  return Result::RESULT_FAIL;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRamIntoBlock::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  _targetID = -1;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRamIntoBlock::TransitionToPuttingDownBlock(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();

  CompoundActionSequential* placeAction = new CompoundActionSequential(robot);
  if(robot.GetCarryingComponent().GetCarryingObject() != _targetID){
    const ObservableObject* obj = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(robot.GetCarryingComponent().GetCarryingObject());
    if(obj != nullptr){
      Vec3f outVector;
      if(ComputeVectorBetween(robot.GetPose(), obj->GetPose(), outVector)){
        Radians angle = FLT_NEAR(outVector.x(), 0.0f) ?
                  Radians(0)                          :
                  Radians(atanf(outVector.y()/outVector.x()));
        const int offsetDirection = angle.ToFloat() > 0 ? 1 : -1;
        angle += DEG_TO_RAD(90 * offsetDirection);
        const bool isAbsolute = true;
        
        // Overshoot the angle to ram by a quarter turn and then place the block down
        placeAction->AddAction(new TurnInPlaceAction(robot, angle.ToFloat(), isAbsolute));
      }
    }
  }
  
  placeAction->AddAction(new PlaceObjectOnGroundAction(robot));
  StartActing(placeAction, &BehaviorRamIntoBlock::TransitionToTurningToBlock);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRamIntoBlock::TransitionToTurningToBlock(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();

  CompoundActionParallel* action = new CompoundActionParallel(robot, {
    new TurnTowardsObjectAction(robot, _targetID),
    new MoveLiftToHeightAction(robot,  MoveLiftToHeightAction::Preset::CARRY)
  });
  StartActing(action, &BehaviorRamIntoBlock::TransitionToRammingIntoBlock);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRamIntoBlock::TransitionToRammingIntoBlock(BehaviorExternalInterface& behaviorExternalInterface)
{
  SmartDisableReactionsWithLock(GetIDStr(), kAffectTriggersRamIntoBlockArray);
  
  const ObservableObject* obj = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_targetID);
  if(obj != nullptr){
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();

    const f32 distToObj = ComputeDistanceBetween(robot.GetPose(), obj->GetPose());
    
    
    CompoundActionParallel* ramAction = new CompoundActionParallel(robot, {
      new DriveStraightAction(robot,
                              distToObj + kDistancePastBlockToDrive_mm,
                              kSpeedToDriveThroughBlock_mmps,
                              false),
      new TriggerAnimationAction(robot, AnimationTrigger::SoundOnlyRamIntoBlock)
    });
    

    IActionRunner* driveOut = new DriveStraightAction(robot,
                                                      -kDistanceBackUpFromBlock_mm,
                                                      kSpeedBackUpFromBlock_mmps);
    
    CompoundActionSequential* driveAction = new CompoundActionSequential(robot);
    driveAction->AddAction(ramAction);
    driveAction->AddAction(driveOut);
    StartActing(driveAction);
  }
}
  
} // namespace Cozmo
} // namespace Anki
