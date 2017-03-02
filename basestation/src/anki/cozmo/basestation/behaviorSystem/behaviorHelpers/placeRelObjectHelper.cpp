/**
* File: placeRelObjectHelper.h
*
* Author: Kevin M. Karol
* Created: 2/21/17
*
* Description: Handles placing a carried block relative to another object
*
* Copyright: Anki, Inc. 2017
*
**/


#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/placeRelObjectHelper.h"

#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/robot.h"


namespace Anki {
namespace Cozmo {

namespace{
static const int kMaxNumRetrys = 3;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PlaceRelObjectHelper::PlaceRelObjectHelper(Robot& robot,
                                           IBehavior& behavior,
                                           BehaviorHelperFactory& helperFactory,
                                           const ObjectID& targetID,
                                           const bool placingOnGround,
                                           const PlaceRelObjectParameters& parameters)
: IHelper("PlaceRelObject", robot, behavior, helperFactory)
, _targetID(targetID)
, _placingOnGround(placingOnGround)
, _params(parameters)
, _tmpRetryCounter(0)
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PlaceRelObjectHelper::~PlaceRelObjectHelper()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PlaceRelObjectHelper::ShouldCancelDelegates(const Robot& robot) const
{
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStatus PlaceRelObjectHelper::Init(Robot& robot)
{
  _tmpRetryCounter = 0;
  const ActionResult isAtPreAction = IsAtPreActionPoseWithVisualVerification(
                    robot, _targetID, PreActionPose::ActionType::PLACE_RELATIVE);
  if(isAtPreAction != ActionResult::SUCCESS){
    DriveToParameters params;
    params.actionType = PreActionPose::ActionType::PLACE_RELATIVE;
    DelegateProperties delegateProperties;
    delegateProperties.SetDelegateToSet(CreateDriveToHelper(robot, _targetID, params));
    delegateProperties.SetOnSuccessFunction([this](Robot& robot){StartPlaceRelObject(robot); return _status;});
    DelegateAfterUpdate(delegateProperties);
  }else{
    StartPlaceRelObject(robot);
  }
  

  return _status;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStatus PlaceRelObjectHelper::UpdateWhileActiveInternal(Robot& robot)
{
  return _status;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PlaceRelObjectHelper::StartPlaceRelObject(Robot& robot)
{
  if(_tmpRetryCounter >= kMaxNumRetrys){
    return;
  }
  _tmpRetryCounter++;

  const ActionResult isAtPreAction = IsAtPreActionPoseWithVisualVerification(
                   robot, _targetID, PreActionPose::ActionType::PLACE_RELATIVE);
  if(isAtPreAction != ActionResult::SUCCESS){
    DriveToParameters params;
    params.actionType = PreActionPose::ActionType::PLACE_RELATIVE;
    params.placeRelOffsetX_mm = _params.placementOffsetX_mm;
    params.placeRelOffsetY_mm = _params.placementOffsetY_mm;
    DelegateProperties properties;
    properties.SetDelegateToSet(CreateDriveToHelper(robot,
                                                    _targetID,
                                                    params));
    properties.SetOnSuccessFunction([this](Robot& robot){
                                      StartPlaceRelObject(robot); return _status;
                                    });
    DelegateAfterUpdate(properties);
  }else{
    PlaceRelObjectAction* driveTo =
            new PlaceRelObjectAction(robot, _targetID, _placingOnGround,
                                     _params.placementOffsetX_mm,
                                     _params.placementOffsetY_mm,
                                     false, _params.relativeCurrentMarker);
    
    StartActing(driveTo, &PlaceRelObjectHelper::RespondToPlaceRelResult);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PlaceRelObjectHelper::RespondToPlaceRelResult(ActionResult result, Robot& robot)
{
  switch(result){
    case ActionResult::SUCCESS:
    {
      _status = BehaviorStatus::Complete;
      break;
    }
    case ActionResult::NO_PREACTION_POSES:
    {
      robot.GetAIComponent().GetWhiteboard().SetNoPreDockPosesOnObject(_targetID);
      break;
    }
    case ActionResult::VISUAL_OBSERVATION_FAILED:
    case ActionResult::LAST_PICK_AND_PLACE_FAILED:
    case ActionResult::DID_NOT_REACH_PREACTION_POSE:
    {
      StartPlaceRelObject(robot);
      break;
    }
    case ActionResult::ABORT:
    case ActionResult::CANCELLED:
    case ActionResult::BAD_OBJECT:
    {
      _status = BehaviorStatus::Failure;
      break;
    }
    default:
    {
      //DEV_ASSERT(false, "HANDLE CASE!");
      //_status = BehaviorStatus::Failure;
      StartPlaceRelObject(robot);
      break;
    }
  }
}


} // namespace Cozmo
} // namespace Anki

