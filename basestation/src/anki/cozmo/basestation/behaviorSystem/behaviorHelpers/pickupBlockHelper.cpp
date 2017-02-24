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
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

namespace{
static const int kMaxNumRetrys = 3;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PickupBlockHelper::PickupBlockHelper(Robot& robot,
                                     IBehavior& behavior,
                                     BehaviorHelperFactory& helperFactory,
                                     const ObjectID& targetID,
                                     AnimationTrigger animBeforeDock)
: IHelper("PickupBlock", robot, behavior, helperFactory)
, _targetID(targetID)
, _animBeforeDock(animBeforeDock)
, _tmpRetryCounter(0)
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
  _tmpRetryCounter = 0;
  StartPickupAction(robot);
  return _status;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStatus PickupBlockHelper::UpdateWhileActiveInternal(Robot& robot)
{
  return _status;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PickupBlockHelper::StartPickupAction(Robot& robot)
{
  if(_tmpRetryCounter >= kMaxNumRetrys){
    return;
  }
  _tmpRetryCounter++;
  
  if(IsAtPreActionPose(robot, _targetID, PreActionPose::ActionType::DOCKING) != ActionResult::SUCCESS){
    DelegateProperties properties;
    properties.SetDelegateToSet(CreateDriveToHelper(robot,
                                                    _targetID,
                                                    PreActionPose::ActionType::DOCKING));
    properties.SetOnSuccessFunction([this](Robot& robot){
                                      StartPickupAction(robot); return _status;
                                   });
    DelegateAfterUpdate(properties);
  }else{
    CompoundActionSequential* action = new CompoundActionSequential(robot);
    if(_animBeforeDock != AnimationTrigger::Count){
      action->AddAction(new TriggerAnimationAction(robot, _animBeforeDock));
      // In case we repeat, null out anim
      _animBeforeDock = AnimationTrigger::Count;
    }
    action->AddAction(new PickupObjectAction(robot, _targetID));
    StartActing(action, &PickupBlockHelper::RespondToPickupResult);
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PickupBlockHelper::RespondToPickupResult(ActionResult result, Robot& robot)
{
  
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
    case ActionResult::DID_NOT_REACH_PREACTION_POSE:
    {
      StartPickupAction(robot);
      break;
    }
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
      StartPickupAction(robot);
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
    default:
    {
      _status = BehaviorStatus::Failure;
      break;
    }
  }
}
  

} // namespace Cozmo
} // namespace Anki

