/**
 * File: rollBlockHelper.h
 *
 * Author: Kevin M. Karol
 * Created: 2/1/17
 *
 * Description: Handles rolling a block
 *
 * Copyright: Anki, Inc. 2017
 *
 **/



#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/rollBlockHelper.h"

#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/behaviorHelperComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/pickupBlockHelper.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/placeBlockHelper.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RollBlockHelper::RollBlockHelper(Robot& robot,
                                 IBehavior* behavior,
                                 BehaviorHelperFactory& helperFactory,
                                 const ObjectID& targetID,
                                 bool rollToUpright)
: IHelper("RollBlock", robot, behavior, helperFactory)
, _targetID(targetID)
, _shouldUpright(rollToUpright)
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RollBlockHelper::~RollBlockHelper()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RollBlockHelper::ShouldCancelDelegates(const Robot& robot) const
{
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStatus RollBlockHelper::Init(Robot& robot,
                                     DelegateProperties& delegateProperties)
{
  // do first update immediately
  _shouldRoll = true;
  return UpdateWhileActiveInternal(robot, delegateProperties);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStatus RollBlockHelper::UpdateWhileActiveInternal(Robot& robot,
                                                          DelegateProperties& delegateProperties)
{
  // If the robot is carrying a block, put it down
  if(robot.IsCarryingObject()){
    delegateProperties.SetDelegateToSet(CreatePlaceBlockHelper(robot));
    delegateProperties.SetOnSuccessFunction(
      [this](Robot& robot) {
        _shouldRoll = true;
        return BehaviorStatus::Running;
      });
    delegateProperties.SetOnFailureFunction([](Robot& robot)
                      { return BehaviorStatus::Failure;});
    return BehaviorStatus::Running;
  }

  if( _shouldRoll ) {
    // If the block can't be accessed, pick it up and move it so it can be rolled
    const ObservableObject* obj = robot.GetBlockWorld().GetObjectByID(_targetID);
    if(obj != nullptr &&
       !obj->IsPoseStateUnknown()){
      const bool canRoll = robot.GetAIComponent().GetWhiteboard().IsObjectValidForAction(
        AIWhiteboard::ObjectUseIntention::RollObjectNoAxisCheck,
        obj->GetID());
      if(canRoll){
        PRINT_CH_INFO("Behaviors", "RollBlockHelper.Update.Rolling", "Doing roll action");
        StartRollingAction(robot);
      }else{
        // If we can't roll the cube, pick it up so we can put it somewhere we can roll it
        auto pickupHelper = CreatePickupBlockHelper(robot, _targetID);
        delegateProperties.SetDelegateToSet(pickupHelper);
        delegateProperties.SetOnFailureFunction( [](Robot& robot)
                                                 {return BehaviorStatus::Failure;});
      }
      
    }else{
      PRINT_CH_INFO("Behaviors", "RollBlockHelper.Update.NoObj", "Failing helper, object %d is invalid",
                    _targetID.GetValue());
      _status = BehaviorStatus::Failure;
    }
  }
  
  return _status;
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RollBlockHelper::StartRollingAction(Robot& robot)
{
  _shouldRoll = false;
  DriveToRollObjectAction* rollAction = new DriveToRollObjectAction(robot, _targetID);
  if( _shouldUpright ) {
    rollAction->RollToUpright();
  }

  StartActing(rollAction, &RollBlockHelper::RespondToRollingResult);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RollBlockHelper::RespondToRollingResult(ActionResult result, Robot& robot)
{
  switch(result){
    case ActionResult::SUCCESS:
    {
      _status = BehaviorStatus::Complete;
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

