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
                                   const PreActionPose::ActionType& actionType)
: IHelper("DriveToHelper", robot, behavior, helperFactory)
, _targetID(targetID)
, _actionType(actionType)
, _searchLevel(0)
, _lastSearchRun_ts(0)
, _tmpRetryCounter(0)
{
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
  
  StartActing(new DriveToObjectAction(robot, _targetID, _actionType),
              &DriveToHelper::RespondToDriveResult);
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

