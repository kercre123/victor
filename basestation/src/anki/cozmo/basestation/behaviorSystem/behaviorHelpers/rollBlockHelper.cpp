/**
 * File: rollBlockHelper.h
 *
 * Author: Kevin M. Karol
 * Created: 2/1/17
 *
 * Description: Handles rolling a block
 *
 * Copyright: Anki, Inc. 2017
 *
 **/



#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/rollBlockHelper.h"

#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/behaviorHelperComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/pickupBlockHelper.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/placeBlockHelper.h"
#include "anki/cozmo/basestation/behaviorSystem/objectInteractionInfoCache.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {
  
namespace{
static const int kMaxNumRetrys = 3;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RollBlockHelper::RollBlockHelper(Robot& robot,
                                 IBehavior& behavior,
                                 BehaviorHelperFactory& helperFactory,
                                 const ObjectID& targetID,
                                 bool rollToUpright,
                                 const RollBlockParameters& parameters)
: IHelper("RollBlock", robot, behavior, helperFactory)
, _targetID(targetID)
, _params(parameters)
, _shouldUpright(rollToUpright)
, _tmpRetryCounter(0)
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RollBlockHelper::~RollBlockHelper()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RollBlockHelper::ShouldCancelDelegates(const Robot& robot) const
{
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStatus RollBlockHelper::Init(Robot& robot)
{
  // do first update immediately
  _shouldRoll = true;
  _tmpRetryCounter = 0;
  DetermineAppropriateAction(robot);
  return _status;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStatus RollBlockHelper::UpdateWhileActiveInternal(Robot& robot)
{
  return _status;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RollBlockHelper::DetermineAppropriateAction(Robot& robot)
{
  // If the robot is carrying a block, put it down
  if(robot.IsCarryingObject()){
    DelegateToPutDown(robot);
  }else{
    // If the block can't be accessed, pick it up and move it so it can be rolled
    const ObservableObject* obj = robot.GetBlockWorld().GetLocatedObjectByID(_targetID);
    if(obj != nullptr){
      const bool canActionRoll = RollObjectAction::CanActionRollObject(robot, obj);

      if(canActionRoll){
        PRINT_CH_INFO("BehaviorHelpers", "RollBlockHelper.Update.Rolling", "Doing roll action");
        const ActionResult isAtPreAction = IsAtPreActionPoseWithVisualVerification(
                                   robot, _targetID, PreActionPose::ActionType::ROLLING);
        
        if(isAtPreAction != ActionResult::SUCCESS){
          DriveToParameters params;
          params.actionType = PreActionPose::ActionType::ROLLING;
          
          // If rolling to upright, set the drive to helper approach angle if possible
          f32 uprightApproachAngle_rad;
          if(_shouldUpright &&
             DriveToRollObjectAction::GetRollToUprightApproachAngle(
                                                    robot,
                                                    _targetID,
                                                    uprightApproachAngle_rad)){
               params.useApproachAngle = true;
               params.approachAngle_rad = uprightApproachAngle_rad;
          }
          
          // Delegate to the drive to helper
          DelegateProperties delegateProperties;
          delegateProperties.SetDelegateToSet(CreateDriveToHelper(robot, _targetID, params));
          delegateProperties.SetOnSuccessFunction([this](Robot& robot)
                                  {StartRollingAction(robot); return _status;}); 
          delegateProperties.SetOnFailureFunction([this](Robot& robot)
                                  {DetermineAppropriateAction(robot); return _status;});
          DelegateAfterUpdate(delegateProperties);
        }else{
          StartRollingAction(robot);
        }
      }else{
        UnableToRollDelegate(robot);
      }
      
    }else{
      PRINT_CH_INFO("BehaviorHelpers",
                    "RollBlockHelper.Update.NoObj",
                    "Failing helper, object %d is invalid",
                    _targetID.GetValue());
      _status = BehaviorStatus::Failure;
    }
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RollBlockHelper::UnableToRollDelegate(Robot& robot)
{
  const ObservableObject* obj = robot.GetBlockWorld().GetLocatedObjectByID(_targetID);
  if(obj != nullptr) {
    auto& objInfoCache = robot.GetAIComponent().GetObjectInteractionInfoCache();

    const bool canPickup = objInfoCache.IsObjectValidForInteraction(
                             ObjectInteractionIntention::PickUpAnyObject,
                             obj->GetID());
    
    // See if any blocks are on top of the one we want to roll
    BlockWorldFilter blocksOnlyFilter;
    blocksOnlyFilter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
    ObservableObject* objOnTop = robot.GetBlockWorld().FindLocatedObjectOnTopOf(
                                              *obj,
                                              BlockWorld::kOnCubeStackHeightTolerance,
                                              blocksOnlyFilter);
    
    if(canPickup){
      PRINT_CH_INFO("BehaviorHelpers", "RollBlockHelper.UnableToRollDelegate.PickingUpTargetObject",
                    "Picking up the target object %d to roll it once moved",
                    obj->GetID().GetValue());
      // If we can pickup the cube, pick it up, put it down somewhere we can roll it
      // and then roll it
      auto pickupHelper = CreatePickupBlockHelper(robot, _targetID);
      DelegateProperties delegateProperties;
      delegateProperties.SetDelegateToSet(pickupHelper);
      delegateProperties.SetOnSuccessFunction([this](Robot& robot)
                                              {DelegateToPutDown(robot); return _status;});

      delegateProperties.SetOnFailureFunction( [](Robot& robot)
                                              {return BehaviorStatus::Failure;});
      DelegateAfterUpdate(delegateProperties);
    }else if(objOnTop != nullptr){
      PRINT_CH_INFO("BehaviorHelpers", "RollBlockHelper.UnableToRollDelegate.PickingUpObjOnTop",
                    "Picking up object %d which is sitting on top of target block",
                    objOnTop->GetID().GetValue());
      // The cube can't be rolled b/c it's under another cube - pickup the top
      // cube and then we can roll the one it's beneath
      auto pickupHelper = CreatePickupBlockHelper(robot, objOnTop->GetID());
      DelegateProperties delegateProperties;
      delegateProperties.SetDelegateToSet(pickupHelper);
      delegateProperties.SetOnSuccessFunction([this](Robot& robot)
                                              {DelegateToPutDown(robot); return _status;});

      delegateProperties.SetOnFailureFunction( [](Robot& robot)
                                              {return BehaviorStatus::Failure;});
      DelegateAfterUpdate(delegateProperties);
    }else{
      PRINT_CH_INFO("BehaviorHelpers", "RollBlockHelper.UnableToRollDelegate.CantInteract",
                    "No way to interact with objID: %d defined",
                    _targetID.GetValue());
    }
  }else{
    PRINT_CH_INFO("BehaviorHelpers", "RollBlockHelper.UnableToRollDelegate.NoObj",
                  "Failing helper, object %d is invalid",
                  _targetID.GetValue());
    _status = BehaviorStatus::Failure;
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RollBlockHelper::DelegateToPutDown(Robot& robot)
{
  DelegateProperties delegateProperties;
  delegateProperties.SetDelegateToSet(CreatePlaceBlockHelper(robot));
  delegateProperties.SetOnSuccessFunction(
                                          [this](Robot& robot) {
                                            _shouldRoll = true;
                                            DetermineAppropriateAction(robot);
                                            return BehaviorStatus::Running;
                                          });
  delegateProperties.SetOnFailureFunction([](Robot& robot)
                                          { return BehaviorStatus::Failure;});
  DelegateAfterUpdate(delegateProperties);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RollBlockHelper::StartRollingAction(Robot& robot)
{
  if(_tmpRetryCounter >= kMaxNumRetrys){
    MarkTargetAsFailedToRoll(robot);
    _status = BehaviorStatus::Failure;
    return;
  }
  _tmpRetryCounter++;
  
  _shouldRoll = false;
  DriveToRollObjectAction* rollAction = new DriveToRollObjectAction(robot, _targetID);
  if( _shouldUpright ) {
    rollAction->RollToUpright();
  }
  if(_params.preDockCallback != nullptr){
    rollAction->SetPreDockCallback(_params.preDockCallback);
  }
  
  rollAction->SetSayNameAnimationTrigger(_params.sayNameAnimationTrigger);
  rollAction->SetNoNameAnimationTrigger(_params.noNameAnimationTrigger);
  rollAction->SetMaxTurnTowardsFaceAngle(_params.maxTurnToFaceAngle);
  

  StartActingWithResponseAnim(rollAction, &RollBlockHelper::RespondToRollingResult, [] (ActionResult result){
    switch(result){
      case ActionResult::SUCCESS:
      {
        return UserFacingActionResult::Success;
        break;
      }
      case ActionResult::LAST_PICK_AND_PLACE_FAILED:
      {
        return UserFacingActionResult::InteractWithBlockDockingIssue;
        break;
      }
      default:
      {
        return UserFacingActionResult::DriveToBlockIssue;
        break;
      }
    }
  });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RollBlockHelper::RespondToRollingResult(ActionResult result, Robot& robot)
{
  switch(result){
    case ActionResult::SUCCESS:
    {
      _status = BehaviorStatus::Complete;
      break;
    }
    case ActionResult::CANCELLED_WHILE_RUNNING:
      // leave the helper running, since it's about to be canceled
      break;
    default:
    {
      StartRollingAction(robot);
      break;
    }
  }
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RollBlockHelper::MarkTargetAsFailedToRoll(Robot& robot)
{
  const ObservableObject* obj = robot.GetBlockWorld().GetLocatedObjectByID(_targetID);
  if(obj != nullptr){
    auto& whiteboard = robot.GetAIComponent().GetWhiteboard();
    whiteboard.SetFailedToUse(*obj, AIWhiteboard::ObjectActionFailure::RollOrPopAWheelie);
  }
}

  
} // namespace Cozmo
} // namespace Anki

