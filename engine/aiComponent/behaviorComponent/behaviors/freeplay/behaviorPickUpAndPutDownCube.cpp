/**
 * File: behaviorPickUpAndPutDownCube.h
 *
 * Author: Brad Neuman
 * Created: 2017-03-06
 *
 * Description: Behavior for the "pickup" spark, which picks up and then puts down a cube
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/behaviorPickUpAndPutDownCube.h"

#include "engine/actions/basicActions.h"
#include "engine/actions/dockActions.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"
#include "engine/components/carryingComponent.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPickUpAndPutDownCube::BehaviorPickUpAndPutDownCube(const Json::Value& config)
: super(config)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPickUpAndPutDownCube::WantsToBeActivatedBehavior() const
{
  auto& objInfoCache = GetBEI().GetAIComponent().GetObjectInteractionInfoCache();
  const ObjectInteractionIntention intent = ObjectInteractionIntention::PickUpObjectNoAxisCheck;
  _targetBlockID = objInfoCache.GetBestObjectForIntention(intent);
  return _targetBlockID.IsSet();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpAndPutDownCube::OnBehaviorActivated()
{
  auto& robotInfo = GetBEI().GetRobotInfo();
  if(robotInfo.GetCarryingComponent().IsCarryingObject()){
    _targetBlockID = robotInfo.GetCarryingComponent().GetCarryingObject();
    TransitionToDriveWithCube();
    
  }
  
  auto& factory = GetBEI().GetAIComponent().GetBehaviorHelperComponent().GetBehaviorHelperFactory();
  PickupBlockParamaters params;
  params.allowedToRetryFromDifferentPose = true;
  HelperHandle pickupHelper = factory.CreatePickupBlockHelper(*this, _targetBlockID, params);
  SmartDelegateToHelper(pickupHelper, &BehaviorPickUpAndPutDownCube::TransitionToDriveWithCube);
  // delegate failure will end the behavior here
  
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpAndPutDownCube::TransitionToDriveWithCube()
{
  DEBUG_SET_STATE(DriveWithCube);
    
  double turn_rad = GetBEI().GetRNG().RandDblInRange(M_PI_4 ,M_PI_F);
  if( GetBEI().GetRNG().RandDbl() < 0.5 )
  {
    turn_rad *= -1;
  }
  
  DelegateIfInControl(new TurnInPlaceAction(turn_rad,false),
              &BehaviorPickUpAndPutDownCube::TransitionToPutDownCube);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpAndPutDownCube::TransitionToPutDownCube()
{
  DEBUG_SET_STATE(PutDownCube);
  
  CompoundActionSequential* action = new CompoundActionSequential();
  {
    PlaceObjectOnGroundAction* placeAction = new PlaceObjectOnGroundAction();
    const bool shouldEmitCompletion = true;
    action->AddAction(placeAction, false, shouldEmitCompletion);
  }

  {  
    static constexpr float kBackUpMinMM = 40.0;
    static constexpr float kBackUpMaxMM = 70.0;
    double backup_amount = GetBEI().GetRNG().RandDblInRange(kBackUpMinMM,kBackUpMaxMM);
    
    action->AddAction( new DriveStraightAction(-backup_amount, DEFAULT_PATH_MOTION_PROFILE.speed_mmps) );
  }

  DelegateIfInControl(action, [this]()
                      {
                        BehaviorObjectiveAchieved(BehaviorObjective::PickedUpAndPutDownBlock);
                        NeedActionCompleted();
                      });
}



}
}



