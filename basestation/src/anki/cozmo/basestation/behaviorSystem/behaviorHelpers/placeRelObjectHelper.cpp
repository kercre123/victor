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
#include "anki/cozmo/basestation/aiComponent/aiComponent.h"
#include "anki/cozmo/basestation/aiComponent/AIWhiteboard.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
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
  
  const PreActionPose::ActionType actionType = PreActionPose::PreActionPose::PLACE_RELATIVE;
  const ActionResult isAtPreAction = IsAtPreActionPoseWithVisualVerification(robot,
                                                                             _targetID,
                                                                             actionType,
                                                                             _params.placementOffsetX_mm,
                                                                             _params.placementOffsetY_mm);
  if(isAtPreAction != ActionResult::SUCCESS){
    DriveToParameters params;
    params.actionType = PreActionPose::ActionType::PLACE_RELATIVE;
    params.placeRelOffsetX_mm = _params.placementOffsetX_mm;
    params.placeRelOffsetY_mm = _params.placementOffsetY_mm;
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
    MarkFailedToStackOrPlace(robot);
    _status = BehaviorStatus::Failure;
    return;
  }
  _tmpRetryCounter++;

  const PreActionPose::ActionType actionType = PreActionPose::PreActionPose::PLACE_RELATIVE;
  const ActionResult isAtPreAction = IsAtPreActionPoseWithVisualVerification(robot,
                                                                             _targetID,
                                                                             actionType,
                                                                             _params.placementOffsetX_mm,
                                                                             _params.placementOffsetY_mm);
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
    PlaceRelObjectAction* placeObj =
            new PlaceRelObjectAction(robot, _targetID, _placingOnGround,
                                     _params.placementOffsetX_mm,
                                     _params.placementOffsetY_mm,
                                     false, _params.relativeCurrentMarker);
    
    {
      // set path motion profile if applicable
      PathMotionProfile mp;
      if(GetPathMotionProfile(robot, mp)){
        placeObj->SetSpeedAndAccel(mp.dockSpeed_mmps,
                                   mp.dockAccel_mmps2,
                                   mp.dockDecel_mmps2);
      }
    }
    
    StartActingWithResponseAnim(placeObj, &PlaceRelObjectHelper::RespondToPlaceRelResult);
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PlaceRelObjectHelper::RespondToPlaceRelResult(ActionResult result, Robot& robot)
{
  switch(result){
    case ActionResult::SUCCESS:
    {
      // Only if stacked directly, not pyramid.
      if(Util::IsNearZero(_params.placementOffsetX_mm) && Util::IsNearZero(_params.placementOffsetY_mm))
      {
        robot.GetContext()->GetNeedsManager()->RegisterNeedsActionCompleted(NeedsActionId::StackCube);
      }
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
    case ActionResult::CANCELLED_WHILE_RUNNING:
      // leave the helper running, since it's about to be canceled
      break;

    case ActionResult::ABORT:
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
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PlaceRelObjectHelper::MarkFailedToStackOrPlace(Robot& robot)
{
  const ObservableObject* placeRelObj = robot.GetBlockWorld().GetLocatedObjectByID(_targetID);
  const ObservableObject* carryingObj = robot.GetBlockWorld().GetLocatedObjectByID(
                                                      robot.GetCarryingObject());
  
  if((placeRelObj != nullptr) &&
     (carryingObj != nullptr)){
    auto& whiteboard = robot.GetAIComponent().GetWhiteboard();
    if(!_placingOnGround){
      // CODE REVIEW - to clarify, should this failure indicate the block that I
      // failed to stack on, or the one in my hand?
      whiteboard.SetFailedToUse(*placeRelObj,
                                AIWhiteboard::ObjectActionFailure::StackOnObject);
      
    }else{
      const f32 xOffset = _params.placementOffsetX_mm;
      const f32 yOffset = _params.placementOffsetY_mm;
      
      const Pose3d& placingAtPose = Pose3d(Z_AXIS_3D(),
                                           {xOffset, yOffset, 0},
                                           &placeRelObj->GetPose());
      whiteboard.SetFailedToUse(*carryingObj,
                                AIWhiteboard::ObjectActionFailure::PlaceObjectAt,
                                placingAtPose);
    }
  }
}



} // namespace Cozmo
} // namespace Anki

