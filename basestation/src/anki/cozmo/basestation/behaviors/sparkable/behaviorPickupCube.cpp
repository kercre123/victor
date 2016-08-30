/**
 * File: behaviorPickupCube.cpp
 *
 * Author: Molly Jameson
 * Created: 2016-07-26
 *
 * Description:  Spark to pick up and put down
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/sparkable/behaviorPickupCube.h"

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/retryWrapperAction.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageFromActiveObject.h"


namespace Anki {
namespace Cozmo {

  
BehaviorPickUpCube::BehaviorPickUpCube(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
{
  SetDefaultName("BehaviorPickUpCube");

  SubscribeToTags({
    EngineToGameTag::RobotObservedObject,
  });

}

Result BehaviorPickUpCube::InitInternal(Robot& robot)
{
  if(robot.IsCarryingObject()){
    TransitionToDriveWithCube(robot);
    return Result::RESULT_OK;
  }
  
  if(!_shouldStreamline){
    TransitionToDoingInitialReaction(robot);
  }else{
    TransitionToPickingUpCube(robot);
  }
  return Result::RESULT_OK;
}

void BehaviorPickUpCube::StopInternal(Robot& robot)
{
  
}

bool BehaviorPickUpCube::IsRunnableInternal(const Robot& robot) const
{
  return _targetBlock.IsSet();
}

void BehaviorPickUpCube::AlwaysHandle(const EngineToGameEvent& event, const Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotObservedObject:
      HandleObjectObserved(robot, event.GetData().Get_RobotObservedObject());
      break;
    default:
      break;
  }
}

void BehaviorPickUpCube::HandleObjectObserved(const Robot& robot, const ExternalInterface::RobotObservedObject& msg)
{
  const ObservableObject* cube =robot.GetBlockWorld().GetObjectByID(msg.objectID);
  if( !_targetBlock.IsSet() && robot.CanPickUpObject(*cube) )
  {
    _targetBlock = msg.objectID;
  }
}
  
void BehaviorPickUpCube::TransitionToDoingInitialReaction(Robot& robot)
{
  DEBUG_SET_STATE(DoingInitialReaction);
  
  // First, always turns towards object to acknowledge it
  // Note: visually verify is true, so if we don't see the object anymore
  // this compound action will fail and we will not contionue this behavior.
  CompoundActionSequential* action = new CompoundActionSequential(robot);
  action->AddAction(new TurnTowardsObjectAction(robot, _targetBlock, Radians(PI_F), true));
  action->AddAction(new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::SparkPickupInitialCubeReaction));
  StartActing(action,
              [this,&robot](ActionResult res) {
                if(ActionResult::SUCCESS != res) {
                  _targetBlock.UnSet();
                } else {
                  TransitionToPickingUpCube(robot);
                }
              });
}
  
void BehaviorPickUpCube::TransitionToPickingUpCube(Robot& robot)
{
  DEBUG_SET_STATE(PickingUpCube);
  DriveToPickupObjectAction* pickupAction = new DriveToPickupObjectAction(robot, _targetBlock, false, 0, false,0, true);
  
  RetryWrapperAction::RetryCallback retryCallback = [this, pickupAction](const ExternalInterface::RobotCompletedAction& completion,
                                                                         const u8 retryCount,
                                                                         AnimationTrigger& retryAnimTrigger)
  {
    retryAnimTrigger = AnimationTrigger::RollBlockRealign;
    
    // Use a different preAction pose if we are retrying because we weren't seeing the object
    if(completion.completionInfo.Get_objectInteractionCompleted().result == ObjectInteractionResult::VISUAL_VERIFICATION_FAILED)
    {
      pickupAction->GetDriveToObjectAction()->SetGetPossiblePosesFunc([this, pickupAction](ActionableObject* object,
                                                                                           std::vector<Pose3d>& possiblePoses,
                                                                                           bool& alreadyInPosition)
      {
        return IBehavior::UseSecondClosestPreActionPose(pickupAction->GetDriveToObjectAction(),
                                                        object, possiblePoses, alreadyInPosition);
      });
    }
    return true;
  };
  
  static const u8 kNumRetries = 1;
  RetryWrapperAction* action = new RetryWrapperAction(robot,
                                                      pickupAction,
                                                      retryCallback,
                                                      kNumRetries);
  
  StartActing(action,
              [this,&robot](ActionResult res) {
                if(ActionResult::SUCCESS != res) {
                  _targetBlock.UnSet();
                } else {
                  StartActing(new TriggerAnimationAction(robot, AnimationTrigger::ReactToBlockPickupSuccess),
                              &BehaviorPickUpCube::TransitionToDriveWithCube);
                }
              });
}
  
void BehaviorPickUpCube::TransitionToDriveWithCube(Robot& robot)
{
  DEBUG_SET_STATE(DriveWithCube);
  
  double turn_rad = robot.GetRNG().RandDblInRange(M_PI_4 ,PI_F);
  if( robot.GetRNG().RandDbl() < 0.5 )
  {
    turn_rad *= -1;
  }
  
  StartActing(new TurnInPlaceAction(robot,Radians(turn_rad),false),
              [this,&robot](ActionResult res) {
                if(ActionResult::SUCCESS != res) {
                  _targetBlock.UnSet();
                } else {
                  TransitionToPutDownCube(robot);
                }
              });
}
  
void BehaviorPickUpCube::TransitionToPutDownCube(Robot& robot)
{
  DEBUG_SET_STATE(PutDownCube);
  
  static constexpr float kBackUpMinMM = 40.0;
  static constexpr float kBackUpMaxMM = 70.0;
  double backup_amount = robot.GetRNG().RandDblInRange(kBackUpMinMM,kBackUpMaxMM);
  StartActing(new CompoundActionSequential(robot, {
    new PlaceObjectOnGroundAction(robot),
    new DriveStraightAction(robot, -backup_amount, DEFAULT_PATH_MOTION_PROFILE.speed_mmps),
  }),
  &BehaviorPickUpCube::TransitionToDoingFinalReaction);
}
  
void BehaviorPickUpCube::TransitionToDoingFinalReaction(Robot& robot)
{
  DEBUG_SET_STATE(DoingFinalReaction);
  
  if(!_shouldStreamline){
    // actively want him to just turn around with the same cube...
    StartActing(new TriggerAnimationAction(robot, AnimationTrigger::SparkPickupFinalCubeReaction),
                [this,&robot](ActionResult res) {
                    // Will no longer be runnable
                    _targetBlock.UnSet();
                    BehaviorObjectiveAchieved(BehaviorObjective::PickedupBlock);
                });
  }else{
    BehaviorObjectiveAchieved(BehaviorObjective::PickedupBlock);
  }
}


} // namespace Cozmo
} // namespace Anki

