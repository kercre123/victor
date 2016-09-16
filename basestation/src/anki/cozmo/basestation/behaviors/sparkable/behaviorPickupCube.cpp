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

static const char* kShouldPutCubeBackDown = "shouldPutCubeBackDown";
static const float kSecondsBetweenBlockWorldChecks = 3;
  
BehaviorPickUpCube::BehaviorPickUpCube(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _lastBlockWorldCheck_s(0)
  , _shouldPutCubeBackDown(true)
{
  SetDefaultName("BehaviorPickUpCube");

  SubscribeToTags({
    EngineToGameTag::RobotObservedObject,
  });
  
  _shouldPutCubeBackDown = config.get(kShouldPutCubeBackDown, true).asBool();

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
  // check even if we haven't seen a block so that we can pickup blocks we know of
  // that are outside FOV
  TimeStamp_t currentTime_s = robot.GetLastMsgTimestamp()/1000;
  if( !IsRunning()
     && currentTime_s > _lastBlockWorldCheck_s + kSecondsBetweenBlockWorldChecks)
  {
    _lastBlockWorldCheck_s = currentTime_s;
    CheckForNearbyObject(robot);
  }
  
  return _targetBlock.IsSet();
}

void BehaviorPickUpCube::HandleWhileNotRunning(const EngineToGameEvent& event, const Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotObservedObject:
      CheckForNearbyObject(robot);
      break;
    default:
      break;
  }
}

void BehaviorPickUpCube::CheckForNearbyObject(const Robot& robot) const
{
  BlockWorldFilter filter;
  filter.SetAllowedFamilies({ObjectFamily::LightCube});
  filter.SetFilterFcn([&robot](const ObservableObject* object)
                      {
                        return robot.CanPickUpObject(*object);
                      });
  
  const ObservableObject* closestObject = robot.GetBlockWorld().FindObjectClosestTo(robot.GetPose(), filter);
  
  if(closestObject != nullptr)
  {
    _targetBlock = closestObject->GetID();
  }
  else
  {
    _targetBlock.UnSet();
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
  if(!_shouldStreamline){
    action->AddAction(new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::SparkPickupInitialCubeReaction));
  }
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
    // This is the intended animation trigger for now - don't change without consulting Mooly
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
  
  if(!_shouldPutCubeBackDown){
    return;
  }
  
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

  CompoundActionSequential* action = new CompoundActionSequential(robot);

  {
    PlaceObjectOnGroundAction* placeAction = new PlaceObjectOnGroundAction(robot);
    const bool shouldEmitCompletion = true;
    action->AddAction(placeAction, false, shouldEmitCompletion);
  }

  {  
    static constexpr float kBackUpMinMM = 40.0;
    static constexpr float kBackUpMaxMM = 70.0;
    double backup_amount = robot.GetRNG().RandDblInRange(kBackUpMinMM,kBackUpMaxMM);
    
    action->AddAction( new DriveStraightAction(robot, -backup_amount, DEFAULT_PATH_MOTION_PROFILE.speed_mmps) );
  }

  
  StartActing(action, &BehaviorPickUpCube::TransitionToDoingFinalReaction);
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

