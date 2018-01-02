/**
 * File: driveToHelper.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2/21/17
 *
 * Description: Handles driving to objects and searching for them if they aren't
 * visually verified
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviorHelpers/driveToHelper.h"

#include "engine/actions/basicActions.h"
#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/actions/visuallyVerifyActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/visionComponent.h"
#include "coretech/common/engine/utils/timer.h"


namespace Anki {
namespace Cozmo {  

namespace{
static const int kMaxNumRetrys = 3;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DriveToHelper::DriveToHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                   ICozmoBehavior& behavior,
                                   BehaviorHelperFactory& helperFactory,
                                   const ObjectID& targetID,
                                   const DriveToParameters& params)
: IHelper("DriveToHelper", behaviorExternalInterface, behavior, helperFactory)
, _targetID(targetID)
, _params(params)
, _tmpRetryCounter(0)
{
  const bool invalidPlaceRelParams =
                (_params.actionType != PreActionPose::PLACE_RELATIVE) &&
                 !(FLT_NEAR(_params.placeRelOffsetY_mm, 0.f) &&
                   FLT_NEAR(_params.placeRelOffsetX_mm, 0.f));
  
  DEV_ASSERT(!invalidPlaceRelParams,"DriveToHelper.InvalidPlaceRelParams");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DriveToHelper::~DriveToHelper()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DriveToHelper::ShouldCancelDelegates(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IHelper::HelperStatus DriveToHelper::InitBehaviorHelper(BehaviorExternalInterface& behaviorExternalInterface)
{
  _initialRobotPose = behaviorExternalInterface.GetRobotInfo().GetPose();
  DriveToPreActionPose(behaviorExternalInterface);
  return _status;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IHelper::HelperStatus DriveToHelper::UpdateWhileActiveInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  return _status;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DriveToHelper::DriveToPreActionPose(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(_tmpRetryCounter >= kMaxNumRetrys){
    _status = IHelper::HelperStatus::Failure;
    return;
  }
  _tmpRetryCounter++;
  
  if((_params.actionType != PreActionPose::PLACE_RELATIVE) ||
     ((_params.placeRelOffsetX_mm == 0) && (_params.placeRelOffsetY_mm == 0))){
    DriveToObjectAction* driveToAction = new DriveToObjectAction(_targetID,
                                                                 _params.actionType);

    if(_params.useApproachAngle){
      driveToAction->SetApproachAngle(_params.approachAngle_rad);
    }
    

    // TODO:(bn) use shared pointers here. This is ok for now because the action must still exist while it's
    // GetPossiblePoses function is called. Even better, make GetPossiblePoses not a member of DriveTo
    if( _params.ignoreCurrentPredockPose ) {
      auto secondClosestPredockFunc = [this,driveToAction](ActionableObject* object,
                                                           std::vector<Pose3d>& possiblePoses,
                                                           bool& alreadyInPosition) {

        ActionResult result = driveToAction->GetPossiblePoses(object, possiblePoses, alreadyInPosition);
        if( result != ActionResult::SUCCESS ) {
          return result;
        }

        if( IDockAction::RemoveMatchingPredockPose(_initialRobotPose, possiblePoses) ) {
          alreadyInPosition = false;
        }
        return ActionResult::SUCCESS;
      };
      
      driveToAction->SetGetPossiblePosesFunc(secondClosestPredockFunc);
    }

    DelegateIfInControl(driveToAction, &DriveToHelper::RespondToDriveResult);
  }else{
    // Calculate the pre-dock pose directly for PLACE_RELATIVE and drive to that pose
    const ActionableObject* obj = dynamic_cast<const ActionableObject*>(
                                    behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_targetID));
    if(obj != nullptr &&
       behaviorExternalInterface.HasVisionComponent()){
      std::vector<Pose3d> possiblePoses;
      bool alreadyInPosition;

      auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
      PlaceRelObjectAction::ComputePlaceRelObjectOffsetPoses(
                              obj, _params.placeRelOffsetX_mm, _params.placeRelOffsetY_mm,
                              robotInfo.GetPose(),
                              robotInfo.GetWorldOrigin(),
                              robotInfo.GetCarryingComponent(),
                              behaviorExternalInterface.GetBlockWorld(),
                              behaviorExternalInterface.GetVisionComponent(),
                              possiblePoses, alreadyInPosition);
      if(possiblePoses.size() > 0){
        if(alreadyInPosition){
          // Already in pose, no drive to necessary
          _status = IHelper::HelperStatus::Complete;
        }else{
          // Drive to the nearest allowed pose, and then perform a visual verify
          CompoundActionSequential* compoundAction = new CompoundActionSequential();
          
          const bool kForceHeadDown = false;
          auto driveToPoseAction = new DriveToPoseAction(possiblePoses, kForceHeadDown);
          
          const bool shouldIgnoreFailure = true;
          compoundAction->AddAction(driveToPoseAction, shouldIgnoreFailure);
          
          compoundAction->AddAction(new VisuallyVerifyObjectAction(_targetID));
          DelegateIfInControl(compoundAction, &DriveToHelper::RespondToDriveResult);
        }
      }else{
        PRINT_NAMED_INFO("DriveToHelper.DriveToPreActionPose.NoPreDockPoses",
                         "No valid predock poses for objectID: %d with offsets x:%f y:%f",
                         _targetID.GetValue(),
                         _params.placeRelOffsetX_mm, _params.placeRelOffsetY_mm);
        behaviorExternalInterface.GetAIComponent().GetWhiteboard().SetNoPreDockPosesOnObject(_targetID);
        _status = IHelper::HelperStatus::Failure;
      }
    }else{
      PRINT_NAMED_WARNING("DriveToHelper.DriveToPreActionPose.TargetBlockNull",
                          "Failed to get ActionableObject for id:%d", _targetID.GetValue());
      _status = IHelper::HelperStatus::Failure;
    }
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DriveToHelper::RespondToDriveResult(ActionResult result, BehaviorExternalInterface& behaviorExternalInterface)
{
  switch(result){
    case ActionResult::SUCCESS:
    {
      _status = IHelper::HelperStatus::Complete;
      break;
    }
    case ActionResult::VISUAL_OBSERVATION_FAILED:
    {
      // If the object is still located search for it
      const auto locatedObj = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_targetID);
      if(locatedObj != nullptr){        
        SearchParameters searchParams;
        searchParams.searchingForID = _targetID;
        DelegateProperties delegateProperties;
        delegateProperties.SetDelegateToSet(CreateSearchForBlockHelper(behaviorExternalInterface, searchParams));
        delegateProperties.SetOnSuccessFunction([this](BehaviorExternalInterface& behaviorExternalInterface){
          DriveToPreActionPose(behaviorExternalInterface);
          return _status;
        });
        delegateProperties.FailImmediatelyOnDelegateFailure();
        DelegateAfterUpdate(delegateProperties);
      }else{
        _status = IHelper::HelperStatus::Failure;
      }
      break;
    }
    case ActionResult::CANCELLED_WHILE_RUNNING:
    {
      // leave the helper running, since it's about to be canceled
      break;
    }
    case ActionResult::NO_PREACTION_POSES:
    {
      if( !_params.ignoreCurrentPredockPose ) {
        behaviorExternalInterface.GetAIComponent().GetWhiteboard().SetNoPreDockPosesOnObject(_targetID);
      }
      _status = IHelper::HelperStatus::Failure;
      break;
    }
    case ActionResult::DID_NOT_REACH_PREACTION_POSE:
    case ActionResult::MOTOR_STOPPED_MAKING_PROGRESS:
    case ActionResult::FAILED_TRAVERSING_PATH:
    {
      DriveToPreActionPose(behaviorExternalInterface);
      break;
    }
    case ActionResult::NOT_STARTED:
    case ActionResult::BAD_OBJECT:
    {
      _status = IHelper::HelperStatus::Failure;
      break;
    }
    case ActionResult::PATH_PLANNING_FAILED_ABORT:
    {
      if (PreActionPose::ActionType::ROLLING == _params.actionType && _params.useApproachAngle) {
        _params.useApproachAngle = false;
        DriveToPreActionPose(behaviorExternalInterface);
      }
      break;
    }
    default:
    {
      //DEV_ASSERT(false, "HANDLE CASE!");
      if( IActionRunner::GetActionResultCategory(result) == ActionResultCategory::RETRY ) {
        DriveToPreActionPose(behaviorExternalInterface);
      }
      else {
        _status = IHelper::HelperStatus::Failure;
      }
      break;
    }
  }
}

  
} // namespace Cozmo
} // namespace Anki

