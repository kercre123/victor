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


#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/driveToHelper.h"

#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/visuallyVerifyActions.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/common/basestation/utils/timer.h"


namespace Anki {
namespace Cozmo {  

namespace{
static const int kMaxNumRetrys = 3;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DriveToHelper::DriveToHelper(Robot& robot,
                                   IBehavior& behavior,
                                   BehaviorHelperFactory& helperFactory,
                                   const ObjectID& targetID,
                                   const DriveToParameters& params)
: IHelper("DriveToHelper", robot, behavior, helperFactory)
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
bool DriveToHelper::ShouldCancelDelegates(const Robot& robot) const
{
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStatus DriveToHelper::Init(Robot& robot)
{
  _initialRobotPose = robot.GetPose();
  DriveToPreActionPose(robot);
  return _status;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStatus DriveToHelper::UpdateWhileActiveInternal(Robot& robot)
{
  return _status;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DriveToHelper::DriveToPreActionPose(Robot& robot)
{
  if(_tmpRetryCounter >= kMaxNumRetrys){
    _status = BehaviorStatus::Failure;
    return;
  }
  _tmpRetryCounter++;
  
  if((_params.actionType != PreActionPose::PLACE_RELATIVE) ||
     ((_params.placeRelOffsetX_mm == 0) && (_params.placeRelOffsetY_mm == 0))){
    DriveToObjectAction* driveToAction = new DriveToObjectAction(robot,
                                                                 _targetID,
                                                                 _params.actionType);

    if(_params.useApproachAngle){
      driveToAction->SetApproachAngle(_params.approachAngle_rad);
    }
    
    {
      PathMotionProfile motionProfile;
      if(GetPathMotionProfile(robot, motionProfile)){
        driveToAction->SetMotionProfile(motionProfile);
      }
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

    StartActing(driveToAction, &DriveToHelper::RespondToDriveResult);
  }else{
    // Calculate the pre-dock pose directly for PLACE_RELATIVE and drive to that pose
    ActionableObject* obj = dynamic_cast<ActionableObject*>(
                           robot.GetBlockWorld().GetLocatedObjectByID(_targetID));
    if(obj != nullptr){
      std::vector<Pose3d> possiblePoses;
      bool alreadyInPosition;
      PlaceRelObjectAction::ComputePlaceRelObjectOffsetPoses(
                              obj, _params.placeRelOffsetX_mm, _params.placeRelOffsetY_mm,
                              robot, possiblePoses, alreadyInPosition);
      if(possiblePoses.size() > 0){
        if(alreadyInPosition){
          // Already in pose, no drive to necessary
          _status = BehaviorStatus::Complete;
        }else{
          // Drive to the nearest allowed pose, and then perform a visual verify
          CompoundActionSequential* compoundAction = new CompoundActionSequential(robot);
          
          auto driveToPoseAction = new DriveToPoseAction(robot, possiblePoses);
          {
            // set path motion profile if applicable
            PathMotionProfile motionProfile;
            if(GetPathMotionProfile(robot, motionProfile)){
              driveToPoseAction->SetMotionProfile(motionProfile);
            }
          }
          const bool shouldIgnoreFailure = true;
          compoundAction->AddAction(driveToPoseAction, shouldIgnoreFailure);
          
          compoundAction->AddAction(new VisuallyVerifyObjectAction(robot, _targetID));
          StartActing(compoundAction, &DriveToHelper::RespondToDriveResult);
        }
      }else{
        PRINT_NAMED_INFO("DriveToHelper.DriveToPreActionPose.NoPreDockPoses",
                         "No valid predock poses for objectID: %d with offsets x:%f y:%f",
                         _targetID.GetValue(),
                         _params.placeRelOffsetX_mm, _params.placeRelOffsetY_mm);
        robot.GetAIComponent().GetWhiteboard().SetNoPreDockPosesOnObject(_targetID);
        _status = BehaviorStatus::Failure;
      }
    }else{
      PRINT_NAMED_WARNING("DriveToHelper.DriveToPreActionPose.TargetBlockNull",
                          "Failed to get ActionableObject for id:%d", _targetID.GetValue());
      _status = BehaviorStatus::Failure;
    }
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DriveToHelper::RespondToDriveResult(ActionResult result, Robot& robot)
{
  switch(result){
    case ActionResult::SUCCESS:
    {
      _status = BehaviorStatus::Complete;
      break;
    }
    case ActionResult::VISUAL_OBSERVATION_FAILED:
    {
      // If the object is still located search for it
      const auto locatedObj = robot.GetBlockWorld().GetLocatedObjectByID(_targetID);
      if(locatedObj != nullptr){        
        SearchParameters searchParams;
        searchParams.searchingForID = _targetID;
        DelegateProperties delegateProperties;
        delegateProperties.SetDelegateToSet(CreateSearchForBlockHelper(robot, searchParams));
        delegateProperties.SetOnSuccessFunction([this](Robot& robot){
          DriveToPreActionPose(robot);
          return _status;
        });
        delegateProperties.FailImmediatelyOnDelegateFailure();
        DelegateAfterUpdate(delegateProperties);
      }else{
        _status = BehaviorStatus::Failure;
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
        robot.GetAIComponent().GetWhiteboard().SetNoPreDockPosesOnObject(_targetID);
      }
      _status = BehaviorStatus::Failure;
      break;
    }
    case ActionResult::DID_NOT_REACH_PREACTION_POSE:
    case ActionResult::MOTOR_STOPPED_MAKING_PROGRESS:
    case ActionResult::FAILED_TRAVERSING_PATH:
    {
      DriveToPreActionPose(robot);
      break;
    }
    case ActionResult::NOT_STARTED:
    case ActionResult::BAD_OBJECT:
    {
      _status = BehaviorStatus::Failure;
      break;
    }
    default:
    {
      //DEV_ASSERT(false, "HANDLE CASE!");
      if( IActionRunner::GetActionResultCategory(result) == ActionResultCategory::RETRY ) {
        DriveToPreActionPose(robot);
      }
      else {
        _status = BehaviorStatus::Failure;
      }
      break;
    }
  }
}

  
} // namespace Cozmo
} // namespace Anki

