/**
 * File: placeBlockHelper.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2/1/17
 *
 * Description: Handles placing a carried block at a given pose
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "engine/behaviorSystem/behaviorHelpers/placeBlockHelper.h"

#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/dockActions.h"
#include "engine/robot.h"


namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PlaceBlockHelper::PlaceBlockHelper(Robot& robot,
                                   IBehavior& behavior,
                                   BehaviorHelperFactory& helperFactory)
: IHelper("PlaceBlock", robot, behavior, helperFactory)
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PlaceBlockHelper::~PlaceBlockHelper()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PlaceBlockHelper::ShouldCancelDelegates(const Robot& robot) const
{
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStatus PlaceBlockHelper::Init(Robot& robot)
{
  double turn_rad = robot.GetRNG().RandDblInRange(M_PI_4 ,M_PI_2);
  if( robot.GetRNG().RandDbl() < 0.5 )
  {
    turn_rad *= -1;
  }
  
  StartActing(new TurnInPlaceAction(robot, (float) turn_rad, false),
              &PlaceBlockHelper::RespondToTurnAction);
  
  return _status;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStatus PlaceBlockHelper::UpdateWhileActiveInternal(Robot& robot)
{
  return _status;
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PlaceBlockHelper::RespondToTurnAction(ActionResult result, Robot& robot)
{
  CompoundActionSequential* action = new CompoundActionSequential(robot);
  
  {
    PlaceObjectOnGroundAction* placeAction = new PlaceObjectOnGroundAction(robot);
    const bool shouldEmitCompletion = true;
    action->AddAction(placeAction, false, shouldEmitCompletion);
  }
  
  // {
  //   static constexpr float kBackUpMinMM = 10.0;
  //   static constexpr float kBackUpMaxMM = 40.0;
  //   double backup_amount = robot.GetRNG().RandDblInRange(kBackUpMinMM,kBackUpMaxMM);
    
  //   action->AddAction( new DriveStraightAction(robot, -backup_amount, DEFAULT_PATH_MOTION_PROFILE.speed_mmps) );
  // }  
  
  StartActing(action, &PlaceBlockHelper::RespondToPlacedAction);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PlaceBlockHelper::RespondToPlacedAction(ActionResult result, Robot& robot)
{
  _status = BehaviorStatus::Complete;
}


} // namespace Cozmo
} // namespace Anki

