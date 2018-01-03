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

#include "engine/aiComponent/behaviorComponent/behaviorHelpers/pickupBlockHelper.h"

#include "engine/actions/animActions.h"
#include "engine/actions/dockActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/behaviorHelperParameters.h"
#include "engine/blockWorld/blockWorld.h"

namespace Anki {
namespace Cozmo {

namespace{
static const int kMaxDockRetries = 2;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PickupBlockHelper::PickupBlockHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                     ICozmoBehavior& behavior,
                                     BehaviorHelperFactory& helperFactory,
                                     const ObjectID& targetID,
                                     const PickupBlockParamaters& parameters)
: IHelper("PickupBlock", behaviorExternalInterface, behavior, helperFactory)
, _targetID(targetID)
, _params(parameters)
, _dockAttemptCount(0)
, _hasTriedOtherPose(false)
{
  
  if(_params.sayNameBeforePickup){
    DEV_ASSERT(!NEAR_ZERO(_params.maxTurnTowardsFaceAngle_rad.ToFloat()),
               "PickupBlockHelper.SayNameButNoTurnAngle");
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PickupBlockHelper::~PickupBlockHelper()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PickupBlockHelper::ShouldCancelDelegates(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return false;
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IHelper::HelperStatus PickupBlockHelper::InitBehaviorHelper(BehaviorExternalInterface& behaviorExternalInterface)
{
  _dockAttemptCount = 0;
  _hasTriedOtherPose = false;
  StartPickupAction(behaviorExternalInterface);
  return _status;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IHelper::HelperStatus PickupBlockHelper::UpdateWhileActiveInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  return _status;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PickupBlockHelper::StartPickupAction(BehaviorExternalInterface& behaviorExternalInterface, bool ignoreCurrentPredockPose)
{
  ActionResult isAtPreAction;
  if( ignoreCurrentPredockPose ) {
    // if we are using second closest we always want to drive. Otherwise, check if we are already in place
    isAtPreAction = ActionResult::ABORT;
  }
  else {
    // if we are using second closest we always want to drive. Otherwise, check if we are already in place
    isAtPreAction = IsAtPreActionPoseWithVisualVerification(behaviorExternalInterface,
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
    properties.SetDelegateToSet(CreateDriveToHelper(behaviorExternalInterface,
                                                    _targetID,
                                                    params));
    properties.SetOnSuccessFunction([this](BehaviorExternalInterface& behaviorExternalInterface){
                                      StartPickupAction(behaviorExternalInterface); return _status;
                                   });
    properties.FailImmediatelyOnDelegateFailure();
    DelegateAfterUpdate(properties);
  }else{

    PRINT_CH_INFO("BehaviorHelpers", "PickupBlockHelper.StartPickupAction.PickingUpObject",
                  "Picking up target object %d",
                  _targetID.GetValue());
    CompoundActionSequential* action = new CompoundActionSequential();
    if(_params.animBeforeDock != AnimationTrigger::Count){
      action->AddAction(new TriggerAnimationAction(_params.animBeforeDock));
      // In case we repeat, null out anim
      _params.animBeforeDock = AnimationTrigger::Count;
    }
    
    if((_dockAttemptCount == 0) &&
       !NEAR_ZERO(_params.maxTurnTowardsFaceAngle_rad.ToFloat())){
      auto turnTowrdsFaceAction = new TurnTowardsLastFacePoseAction(_params.maxTurnTowardsFaceAngle_rad,
                                                                    _params.sayNameBeforePickup);
      turnTowrdsFaceAction->SetSayNameAnimationTrigger(AnimationTrigger::PickupHelperPreActionNamedFace);
      turnTowrdsFaceAction->SetNoNameAnimationTrigger(AnimationTrigger::PickupHelperPreActionUnnamedFace);
      static const bool ignoreFailure = true;
      action->AddAction(turnTowrdsFaceAction,
                        ignoreFailure);
      
      action->AddAction(new TurnTowardsObjectAction(_targetID,
                                                    M_PI_F),
                        ignoreFailure);
    }

    {
      PickupObjectAction* pickupAction = new PickupObjectAction(_targetID);
      // no need to do an extra check in the action
      pickupAction->SetDoNearPredockPoseCheck(false);
      
      action->AddAction(pickupAction);
      action->SetProxyTag(pickupAction->GetTag());
    }
    
    DelegateWithResponseAnim(action, &PickupBlockHelper::RespondToPickupResult,  [] (ActionResult result){
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
    _dockAttemptCount++;
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PickupBlockHelper::RespondToPickupResult(const ExternalInterface::RobotCompletedAction& rca,
                                              BehaviorExternalInterface& behaviorExternalInterface)
{
  const ActionResult& result = rca.result;
  PRINT_CH_DEBUG("BehaviorHelpers", (GetName() + ".PickupResult").c_str(),
                 "%s", ActionResultToString(result));
    
  switch(result){
    case ActionResult::SUCCESS:
    {
      _status = IHelper::HelperStatus::Complete;
      break;
    }
    case ActionResult::VISUAL_OBSERVATION_FAILED:
    {
      DEV_ASSERT_MSG(rca.completionInfo.GetTag() == ActionCompletedUnionTag::objectInteractionCompleted,
                 "PickupBlockHelper.RespondToPickupResult.UnexpectedActionCompletedUnionTag",
                 "%hhu", rca.completionInfo.GetTag());
      
      if(rca.completionInfo.Get_objectInteractionCompleted().seeingUnexpectedObject)
      {
        PRINT_CH_DEBUG("BehaviorHelpers",
                       (GetName() + ".VisualObservationFailed.SeeingUnexpectedObject").c_str(),
                       "Marking target as failed to pickup");
        
        MarkTargetAsFailedToPickup(behaviorExternalInterface);
        _status = IHelper::HelperStatus::Failure;
      }
      else
      {
        SearchParameters params;
        params.searchingForID = _targetID;
        params.searchIntensity = SearchIntensity::QuickSearch;
        DelegateProperties properties;
        properties.SetDelegateToSet(CreateSearchForBlockHelper(behaviorExternalInterface, params));
        properties.SetOnSuccessFunction([this](BehaviorExternalInterface& behaviorExternalInterface){
          StartPickupAction(behaviorExternalInterface); return _status;
        });
        properties.SetOnFailureFunction([this](BehaviorExternalInterface& behaviorExternalInterface){
          MarkTargetAsFailedToPickup(behaviorExternalInterface); return IHelper::HelperStatus::Failure;
        });
        DelegateAfterUpdate(properties);
      }
      break;
    }
    case ActionResult::NO_PREACTION_POSES:
    {
      behaviorExternalInterface.GetAIComponent().GetWhiteboard().SetNoPreDockPosesOnObject(_targetID);
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
        StartPickupAction(behaviorExternalInterface);
      }
      else if( _params.allowedToRetryFromDifferentPose && !_hasTriedOtherPose ) {
        PRINT_CH_INFO("BehaviorHelpers", (GetName() + ".RetryFromOtherPose").c_str(),
                      "Trying again with a different predock pose");
        _dockAttemptCount = 0;
        _hasTriedOtherPose = true;
        const bool ignoreCurrentPredockPose = true;
        StartPickupAction(behaviorExternalInterface, ignoreCurrentPredockPose);
      }
      else {
        PRINT_CH_INFO("BehaviorHelpers", (GetName() + ".PickupFailedTooManyTimes").c_str(),
                      "Failing helper because pickup was already attempted %d times",
                      _dockAttemptCount);
        MarkTargetAsFailedToPickup(behaviorExternalInterface);
        _status = IHelper::HelperStatus::Failure;
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
      _status = IHelper::HelperStatus::Failure;
      break;
    }

    case ActionResult::DID_NOT_REACH_PREACTION_POSE:
    {
      // DriveToHelper should handle this, shouldn't see it here
      PRINT_NAMED_ERROR("PickupBlockHelper.InvalidPickupResponse", "%s", ActionResultToString(result));
      _status = IHelper::HelperStatus::Failure;
      break;
    }

    default:
    {
      //DEV_ASSERT(false, "HANDLE CASE!");
      if( IActionRunner::GetActionResultCategory(result) == ActionResultCategory::RETRY ) {
        StartPickupAction(behaviorExternalInterface);
      }
      else {
        MarkTargetAsFailedToPickup(behaviorExternalInterface);
        _status = IHelper::HelperStatus::Failure;
      }
      break;
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PickupBlockHelper::MarkTargetAsFailedToPickup(BehaviorExternalInterface& behaviorExternalInterface)
{
  const ObservableObject* obj = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_targetID);
  if(obj != nullptr){
    auto& whiteboard = behaviorExternalInterface.GetAIComponent().GetWhiteboard();
    whiteboard.SetFailedToUse(*obj, AIWhiteboard::ObjectActionFailure::PickUpObject);
  }
}


} // namespace Cozmo
} // namespace Anki

