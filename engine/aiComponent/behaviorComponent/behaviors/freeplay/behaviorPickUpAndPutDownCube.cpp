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
#include "engine/aiComponent/AIWhiteboard.h"
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
bool BehaviorPickUpAndPutDownCube::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  auto& objInfoCache = behaviorExternalInterface.GetAIComponent().GetObjectInteractionInfoCache();
  const ObjectInteractionIntention intent = ObjectInteractionIntention::PickUpObjectNoAxisCheck;
  _targetBlockID = objInfoCache.GetBestObjectForIntention(intent);
  return _targetBlockID.IsSet();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpAndPutDownCube::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
  if(robotInfo.GetCarryingComponent().IsCarryingObject()){
    _targetBlockID = robotInfo.GetCarryingComponent().GetCarryingObject();
    TransitionToDriveWithCube(behaviorExternalInterface);
    
  }
  
  auto& factory = behaviorExternalInterface.GetAIComponent().GetBehaviorHelperComponent().GetBehaviorHelperFactory();
  PickupBlockParamaters params;
  params.allowedToRetryFromDifferentPose = true;
  HelperHandle pickupHelper = factory.CreatePickupBlockHelper(behaviorExternalInterface, *this, _targetBlockID, params);
  SmartDelegateToHelper(behaviorExternalInterface, pickupHelper, &BehaviorPickUpAndPutDownCube::TransitionToDriveWithCube);
  // delegate failure will end the behavior here
  
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpAndPutDownCube::TransitionToDriveWithCube(BehaviorExternalInterface& behaviorExternalInterface)
{
  DEBUG_SET_STATE(DriveWithCube);
    
  double turn_rad = behaviorExternalInterface.GetRNG().RandDblInRange(M_PI_4 ,M_PI_F);
  if( behaviorExternalInterface.GetRNG().RandDbl() < 0.5 )
  {
    turn_rad *= -1;
  }
  
  DelegateIfInControl(new TurnInPlaceAction(turn_rad,false),
              &BehaviorPickUpAndPutDownCube::TransitionToPutDownCube);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPickUpAndPutDownCube::TransitionToPutDownCube(BehaviorExternalInterface& behaviorExternalInterface)
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
    double backup_amount = behaviorExternalInterface.GetRNG().RandDblInRange(kBackUpMinMM,kBackUpMaxMM);
    
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



