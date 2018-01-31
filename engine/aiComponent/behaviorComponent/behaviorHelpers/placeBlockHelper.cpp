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

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PlaceBlockHelper::PlaceBlockHelper(ICozmoBehavior& behavior,
                                   BehaviorHelperFactory& helperFactory)
: IHelper("PlaceBlock", behavior, helperFactory)
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PlaceBlockHelper::~PlaceBlockHelper()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PlaceBlockHelper::ShouldCancelDelegates() const
{
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IHelper::HelperStatus PlaceBlockHelper::InitBehaviorHelper()
{
  double turn_rad = GetBEI().GetRNG().RandDblInRange(M_PI_4 ,M_PI_2);
  if( GetBEI().GetRNG().RandDbl() < 0.5 )
  {
    turn_rad *= -1;
  }
  {
    DelegateIfInControl(new TurnInPlaceAction((float) turn_rad, false),
                        &PlaceBlockHelper::RespondToTurnAction);
  }

  
  return _status;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IHelper::HelperStatus PlaceBlockHelper::UpdateWhileActiveInternal()
{
  return _status;
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PlaceBlockHelper::RespondToTurnAction(ActionResult result)
{
  CompoundActionSequential* action = new CompoundActionSequential();
  
  {
    PlaceObjectOnGroundAction* placeAction = new PlaceObjectOnGroundAction();
    const bool shouldEmitCompletion = true;
    action->AddAction(placeAction, false, shouldEmitCompletion);
  }
  
  // {
  //   static constexpr float kBackUpMinMM = 10.0;
  //   static constexpr float kBackUpMaxMM = 40.0;
  //   double backup_amount = robot.GetRNG().RandDblInRange(kBackUpMinMM,kBackUpMaxMM);
    
  //   action->AddAction( new DriveStraightAction(-backup_amount, DEFAULT_PATH_MOTION_PROFILE.speed_mmps) );
  // }  
  
  DelegateIfInControl(action, &PlaceBlockHelper::RespondToPlacedAction);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PlaceBlockHelper::RespondToPlacedAction(ActionResult result)
{
  _status = IHelper::HelperStatus::Complete;
}


} // namespace Cozmo
} // namespace Anki

