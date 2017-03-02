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
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
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
, _searchLevel(0)
, _lastSearchRun_ts(0)
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
  _tmpRetryCounter = 0;
  _searchLevel = 0;
  _lastSearchRun_ts = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
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
    return;
  }
  _tmpRetryCounter++;
  
  if((_params.actionType != PreActionPose::PLACE_RELATIVE) ||
     ((_params.placeRelOffsetX_mm == 0) && (_params.placeRelOffsetY_mm == 0))){
    StartActing(new DriveToObjectAction(robot, _targetID, _params.actionType),
                &DriveToHelper::RespondToDriveResult);
  }else{
    // Calculate the pre-dock pose directly for PLACE_RELATIVE and drive to that pose
    ActionableObject* obj = dynamic_cast<ActionableObject*>(
                           robot.GetBlockWorld().GetObjectByID(_targetID));
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
          // Drive to the nearest pose
          StartActing(new DriveToPoseAction(robot, possiblePoses),
                      &DriveToHelper::RespondToDriveResult);
        }
      }else{
        PRINT_NAMED_INFO("DriveToHelper.DriveToPreActionPose.NoPreDockPoses",
                         "No valid predock poses for objectID: %d with offsets x:%f y:%f",
                         _targetID.GetValue(),
                         _params.placeRelOffsetX_mm, _params.placeRelOffsetY_mm);
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
      _lastSearchRun_ts = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
      SearchForBlock(result, robot);
      break;
    }
    case ActionResult::CANCELLED:
    {
      _status = BehaviorStatus::Failure;
      break;
    }
    case ActionResult::NO_PREACTION_POSES:
    {
      robot.GetAIComponent().GetWhiteboard().SetNoPreDockPosesOnObject(_targetID);
      break;
    }
    case ActionResult::DID_NOT_REACH_PREACTION_POSE:
    case ActionResult::MOTOR_STOPPED_MAKING_PROGRESS:
    {
      DriveToPreActionPose(robot);
      break;
    }
    case ActionResult::NOT_STARTED:
    case ActionResult::BAD_OBJECT:
    case ActionResult::FAILED_TRAVERSING_PATH:
    {
      _status = BehaviorStatus::Failure;
      break;
    }
    default:
    {
      //DEV_ASSERT(false, "HANDLE CASE!");
      DriveToPreActionPose(robot);
      break;
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DriveToHelper::SearchForBlock(ActionResult result, Robot& robot)
{
  
  const ObservableObject* staticBlock = robot.GetBlockWorld().GetObjectByID(_targetID);
  if(staticBlock != nullptr &&
     !staticBlock->IsPoseStateUnknown()){
    // Check if block observed since last search
    const TimeStamp_t lastObserved = staticBlock->GetLastObservedTime();
    if(lastObserved > _lastSearchRun_ts){
      _status = BehaviorStatus::Complete;
      return;
    }
    
    // Increment search lewel
    switch(_searchLevel){
      case 0:
      {
        auto searchNearby = new SearchForNearbyObjectAction(robot, _targetID);
        
        CompoundActionSequential* compoundAction = new CompoundActionSequential(robot);
        compoundAction->AddAction(new TurnTowardsObjectAction(robot, _targetID, M_PI), true);
        compoundAction->AddAction(searchNearby);
        StartActing(compoundAction, &DriveToHelper::SearchForBlock);
        
        break;
      }
      case 1:
      {
        CompoundActionSequential* compoundAction = new CompoundActionSequential(robot);
        compoundAction->AddAction(new TurnInPlaceAction(robot, M_PI_4, false));
        compoundAction->AddAction(new TurnInPlaceAction(robot, M_PI_4, false));
        compoundAction->AddAction(new TurnInPlaceAction(robot, -M_PI_2, false));
        compoundAction->AddAction(new TurnInPlaceAction(robot, -M_PI_4, false));
        compoundAction->AddAction(new TurnInPlaceAction(robot, -M_PI_4, false));

        StartActing(compoundAction, &DriveToHelper::SearchForBlock);
        break;
      }
      default:
      {
        if(staticBlock->IsPoseStateKnown()){
          PRINT_NAMED_ERROR("DriveToHelper.SearchForBlock.GoingNuclear",
                              "Failed to find known block - wiping");
          robot.GetBlockWorld().ClearAllExistingObjects();
        }
        _status = BehaviorStatus::Failure;
      }
    }
  }else{
    _status = BehaviorStatus::Complete;
  }
  
  _searchLevel++;
  _lastSearchRun_ts = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
}
  
  
} // namespace Cozmo
} // namespace Anki

