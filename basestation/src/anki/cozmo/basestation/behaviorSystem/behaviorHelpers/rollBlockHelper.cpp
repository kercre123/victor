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

#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/behaviorHelperComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/pickupBlockHelper.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/placeBlockHelper.h"
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
      auto& whiteboard = robot.GetAIComponent().GetWhiteboard();
      const bool canRoll = whiteboard.IsObjectValidForAction(
                               AIWhiteboard::ObjectUseIntention::RollObjectNoAxisCheck,
                               obj->GetID());
      if(canRoll){
        PRINT_CH_INFO("Behaviors", "RollBlockHelper.Update.Rolling", "Doing roll action");
        const ActionResult isAtPreAction = IsAtPreActionPoseWithVisualVerification(
                                   robot, _targetID, PreActionPose::ActionType::ROLLING);
        
        if(isAtPreAction != ActionResult::SUCCESS){
          DriveToParameters params;
          params.actionType = PreActionPose::ActionType::ROLLING;
          DelegateProperties delegateProperties;
          delegateProperties.SetDelegateToSet(CreateDriveToHelper(robot, _targetID, params));
          delegateProperties.SetOnSuccessFunction([this](Robot& robot){StartRollingAction(robot); return _status;});
          DelegateAfterUpdate(delegateProperties);
        }else{
          StartRollingAction(robot);
        }
      }else{
        // If we can't roll the cube, pick it up so we can put it somewhere we can roll it
        auto pickupHelper = CreatePickupBlockHelper(robot, _targetID);
        DelegateProperties delegateProperties;
        delegateProperties.SetDelegateToSet(pickupHelper);
        delegateProperties.SetOnFailureFunction( [](Robot& robot)
                                                {return BehaviorStatus::Failure;});
        DelegateAfterUpdate(delegateProperties);
      }
      
    }else{
      PRINT_CH_INFO("Behaviors", "RollBlockHelper.Update.NoObj", "Failing helper, object %d is invalid",
                    _targetID.GetValue());
      _status = BehaviorStatus::Failure;
    }
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
    case ActionResult::CANCELLED:
      // leave the helper running, since it's about to be canceled
      break;
    default:
    {
      StartRollingAction(robot);
      break;
    }
  }
}

  
} // namespace Cozmo
} // namespace Anki

