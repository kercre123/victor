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
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"

namespace Anki {
namespace Cozmo {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PickupBlockHelper::PickupBlockHelper(Robot& robot,
                                     IBehavior* behavior,
                                     BehaviorHelperFactory& helperFactory,
                                     const ObjectID& targetID)
: IHelper("PickupBlock", robot, behavior, helperFactory)
, _targetID(targetID)
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
BehaviorStatus PickupBlockHelper::Init(Robot& robot,
                                       DelegateProperties& delegateProperties)
{
  StartPickupAction(robot);
  return _status;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStatus PickupBlockHelper::UpdateWhileActiveInternal(Robot& robot,
                                                            DelegateProperties& delegateProperties)
{
  return _status;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PickupBlockHelper::StartPickupAction(Robot& robot)
{
  IActionRunner* pickupAction = new DriveToPickupObjectAction(robot, _targetID);
  StartActing(pickupAction, &PickupBlockHelper::RespondToPickupResult);
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
    default:
    {
      _status = BehaviorStatus::Failure;
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

