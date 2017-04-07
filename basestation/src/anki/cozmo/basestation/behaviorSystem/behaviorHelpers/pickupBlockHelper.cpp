/**
 * File: pickupBlockHelper.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2/1/17
 *
 * Description: Handles picking up a block with a given ID
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/pickupBlockHelper.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/behaviorHelperParameters.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

namespace{
  static const int kMaxDockRetries = 2;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PickupBlockHelper::PickupBlockHelper(Robot& robot,
                                     IBehavior& behavior,
                                     BehaviorHelperFactory& helperFactory,
                                     const ObjectID& targetID,
                                     const PickupBlockParamaters& parameters)
: IHelper("PickupBlock", robot, behavior, helperFactory)
, _targetID(targetID)
, _params(parameters)
, _dockAttemptCount(0)
, _hasTriedOtherPose(false)
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PickupBlockHelper::~PickupBlockHelper()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PickupBlockHelper::ShouldCancelDelegates(const Robot& robot) const
{
  return false;
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStatus PickupBlockHelper::Init(Robot& robot)
{
  _dockAttemptCount = 0;
  _hasTriedOtherPose = false;
  StartPickupAction(robot);
  return _status;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStatus PickupBlockHelper::UpdateWhileActiveInternal(Robot& robot)
{
  return _status;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PickupBlockHelper::StartPickupAction(Robot& robot, bool ignoreCurrentPredockPose)
{
  ActionResult isAtPreAction;
  if( ignoreCurrentPredockPose ) {
    // if we are using second closest we always want to drive. Otherwise, check if we are already in place
    isAtPreAction = ActionResult::ABORT;
  }
  else {
    // if we are using second closest we always want to drive. Otherwise, check if we are already in place
    isAtPreAction = IsAtPreActionPoseWithVisualVerification(robot,
                                                            _targetID,
                                                            PreActionPose::ActionType::DOCKING);
  }
  
  if(isAtPreAction != ActionResult::SUCCESS){
    PRINT_CH_INFO("BehaviorHelpers", "PickupBlockHelper.StartPickupAction.DrivingToPreDockPose",
                  "Cozmo is not at pre-action pose for cube %d, delegating to driveToHelper",
                  _targetID.GetValue());
    DriveToParameters params;
    params.actionType = PreActionPose::ActionType::DOCKING;
    params.ignoreCurrentPredockPose = ignoreCurrentPredockPose;
    DelegateProperties properties;
    properties.SetDelegateToSet(CreateDriveToHelper(robot,
                                                    _targetID,
                                                    params));
    properties.SetOnSuccessFunction([this](Robot& robot){
                                      StartPickupAction(robot); return _status;                                      
                                   });
    properties.FailImmediatelyOnDelegateFailure();
    DelegateAfterUpdate(properties);
  }else{
    PRINT_CH_INFO("BehaviorHelpers", "PickupBlockHelper.StartPickupAction.PickingUpObject",
                  "Picking up target object %d",
                  _targetID.GetValue());
    CompoundActionSequential* action = new CompoundActionSequential(robot);
    if(_params.animBeforeDock != AnimationTrigger::Count){
      action->AddAction(new TriggerAnimationAction(robot, _params.animBeforeDock));
      // In case we repeat, null out anim
      _params.animBeforeDock = AnimationTrigger::Count;
    }

    _dockAttemptCount++;

    {
      PickupObjectAction* pickupAction = new PickupObjectAction(robot, _targetID);
      // no need to do an extra check in the action
      pickupAction->SetDoNearPredockPoseCheck(false);
      action->AddAction(pickupAction);
    }
    
    StartActingWithResponseAnim(action, &PickupBlockHelper::RespondToPickupResult,  [] (ActionResult result){
      switch(result){
        case ActionResult::SUCCESS:
        {
          return UserFacingActionResult::Count;
          break;
        }
        case ActionResult::MOTOR_STOPPED_MAKING_PROGRESS:
        case ActionResult::NOT_CARRYING_OBJECT_RETRY:
        case ActionResult::PICKUP_OBJECT_UNEXPECTEDLY_NOT_MOVING:
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
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PickupBlockHelper::RespondToPickupResult(ActionResult result, Robot& robot)
{
  PRINT_CH_DEBUG("BehaviorHelpers", (GetName() + ".PickupResult").c_str(),
                 "%s", ActionResultToString(result));
    
  switch(result){
    case ActionResult::SUCCESS:
    {
      _status = BehaviorStatus::Complete;
      break;
    }
    case ActionResult::VISUAL_OBSERVATION_FAILED:
    {
      IActionRunner* searchAction = new SearchForNearbyObjectAction(robot, _targetID);
      StartActing(searchAction, &PickupBlockHelper::RespondToSearchResult);
      break;
    }
    case ActionResult::NO_PREACTION_POSES:
    {
      robot.GetAIComponent().GetWhiteboard().SetNoPreDockPosesOnObject(_targetID);
      break;
    }
    case ActionResult::MOTOR_STOPPED_MAKING_PROGRESS:
    case ActionResult::NOT_CARRYING_OBJECT_RETRY:
    case ActionResult::PICKUP_OBJECT_UNEXPECTEDLY_NOT_MOVING:
    case ActionResult::LAST_PICK_AND_PLACE_FAILED:
    {
      PRINT_CH_INFO("BehaviorHelpers", (GetName() + ".DockAttemptFailed").c_str(),
                    "Failed dock attempt %d / %d",
                    _dockAttemptCount,
                    kMaxDockRetries);                                        
      
      if( _dockAttemptCount < kMaxDockRetries ) {
        StartPickupAction(robot);
      }
      else if( _params.allowedToRetryFromDifferentPose && !_hasTriedOtherPose ) {
        PRINT_CH_INFO("BehaviorHelpers", (GetName() + ".RetryFromOtherPose").c_str(),
                      "Trying again with a different predock pose");
        _dockAttemptCount = 0;
        _hasTriedOtherPose = true;
        const bool ignoreCurrentPredockPose = true;
        StartPickupAction(robot, ignoreCurrentPredockPose);
      }
      else {
        PRINT_CH_INFO("BehaviorHelpers", (GetName() + ".PickupFailedTooManyTimes").c_str(),
                      "Failing helper because pickup was already attempted %d times",
                      _dockAttemptCount);
        MarkTargetAsFailedToPickup(robot);
        _status = BehaviorStatus::Failure;
      }
      break;
    }

    case ActionResult::CANCELLED_WHILE_RUNNING:
    {
      // leave the helper running, since it's about to be canceled
      break;
    }
    case ActionResult::BAD_OBJECT:
    {
      _status = BehaviorStatus::Failure;
      break;
    }

    case ActionResult::DID_NOT_REACH_PREACTION_POSE:
    {
      // DriveToHelper should handle this, shouldn't see it here
      PRINT_NAMED_ERROR("PickupBlockHelper.InvalidPickupResponse", "%s", ActionResultToString(result));
      _status = BehaviorStatus::Failure;
      break;
    }

    default:
    {
      //DEV_ASSERT(false, "HANDLE CASE!");
      if( IActionRunner::GetActionResultCategory(result) == ActionResultCategory::RETRY ) {
        StartPickupAction(robot);
      }
      else {
        MarkTargetAsFailedToPickup(robot);
        _status = BehaviorStatus::Failure;
      }
      break;
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PickupBlockHelper::RespondToSearchResult(ActionResult result, Robot& robot)
{
  switch(result){
    case ActionResult::SUCCESS:
    {
      StartPickupAction(robot);
      break;
    }
    case ActionResult::CANCELLED_WHILE_RUNNING:
      // leave the helper running, since it's about to be canceled
      break;
    default:
    {
      _status = BehaviorStatus::Failure;
      break;
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PickupBlockHelper::MarkTargetAsFailedToPickup(Robot& robot)
{
  const ObservableObject* obj = robot.GetBlockWorld().GetLocatedObjectByID(_targetID);
  if(obj != nullptr){
    auto& whiteboard = robot.GetAIComponent().GetWhiteboard();
    whiteboard.SetFailedToUse(*obj, AIWhiteboard::ObjectActionFailure::PickUpObject);
  }
}


} // namespace Cozmo
} // namespace Anki

