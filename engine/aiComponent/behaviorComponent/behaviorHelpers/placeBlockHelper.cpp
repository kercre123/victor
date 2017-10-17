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


#include "engine/aiComponent/behaviorComponent/behaviorHelpers/placeBlockHelper.h"

#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/dockActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/robot.h"


namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PlaceBlockHelper::PlaceBlockHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                   ICozmoBehavior& behavior,
                                   BehaviorHelperFactory& helperFactory)
: IHelper("PlaceBlock", behaviorExternalInterface, behavior, helperFactory)
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PlaceBlockHelper::~PlaceBlockHelper()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PlaceBlockHelper::ShouldCancelDelegates(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStatus PlaceBlockHelper::InitBehaviorHelper(BehaviorExternalInterface& behaviorExternalInterface)
{
  double turn_rad = behaviorExternalInterface.GetRNG().RandDblInRange(M_PI_4 ,M_PI_2);
  if( behaviorExternalInterface.GetRNG().RandDbl() < 0.5 )
  {
    turn_rad *= -1;
  }
  {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    DelegateIfInControl(new TurnInPlaceAction(robot, (float) turn_rad, false),
                &PlaceBlockHelper::RespondToTurnAction);
  }

  
  return _status;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStatus PlaceBlockHelper::UpdateWhileActiveInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  return _status;
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PlaceBlockHelper::RespondToTurnAction(ActionResult result, BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
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
  
  DelegateIfInControl(action, &PlaceBlockHelper::RespondToPlacedAction);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PlaceBlockHelper::RespondToPlacedAction(ActionResult result, BehaviorExternalInterface& behaviorExternalInterface)
{
  _status = BehaviorStatus::Complete;
}


} // namespace Cozmo
} // namespace Anki

