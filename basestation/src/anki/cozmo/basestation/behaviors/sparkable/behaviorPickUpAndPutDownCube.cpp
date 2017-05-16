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

#include "anki/cozmo/basestation/behaviors/sparkable/behaviorPickUpAndPutDownCube.h"

#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/behaviorHelperComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/behaviorSystem/objectInteractionInfoCache.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

BehaviorPickUpAndPutDownCube::BehaviorPickUpAndPutDownCube(Robot& robot, const Json::Value& config)
  : super(robot, config)
{
}

bool BehaviorPickUpAndPutDownCube::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  auto& objInfoCache = preReqData.GetRobot().GetAIComponent().GetObjectInteractionInfoCache();
  const ObjectInteractionIntention intent = ObjectInteractionIntention::PickUpObjectNoAxisCheck;
  _targetBlockID = objInfoCache.GetBestObjectForIntention(intent);
  return _targetBlockID.IsSet();
}

Result BehaviorPickUpAndPutDownCube::InitInternal(Robot& robot)
{
  if(robot.IsCarryingObject()){
    _targetBlockID = robot.GetCarryingObject();
    TransitionToDriveWithCube(robot);
    return Result::RESULT_OK;
  }
  
  auto& factory = robot.GetAIComponent().GetBehaviorHelperComponent().GetBehaviorHelperFactory();
  PickupBlockParamaters params;
  params.allowedToRetryFromDifferentPose = true;
  HelperHandle pickupHelper = factory.CreatePickupBlockHelper(robot, *this, _targetBlockID, params);
  SmartDelegateToHelper(robot, pickupHelper, &BehaviorPickUpAndPutDownCube::TransitionToDriveWithCube);
  // delegate failure will end the behavior here
  
  return Result::RESULT_OK;
}

void BehaviorPickUpAndPutDownCube::TransitionToDriveWithCube(Robot& robot)
{
  DEBUG_SET_STATE(DriveWithCube);
    
  double turn_rad = robot.GetRNG().RandDblInRange(M_PI_4 ,M_PI_F);
  if( robot.GetRNG().RandDbl() < 0.5 )
  {
    turn_rad *= -1;
  }
  
  StartActing(new TurnInPlaceAction(robot,turn_rad,false),
              &BehaviorPickUpAndPutDownCube::TransitionToPutDownCube);
}
  
void BehaviorPickUpAndPutDownCube::TransitionToPutDownCube(Robot& robot)
{
  DEBUG_SET_STATE(PutDownCube);
  
  CompoundActionSequential* action = new CompoundActionSequential(robot);

  {
    PlaceObjectOnGroundAction* placeAction = new PlaceObjectOnGroundAction(robot);
    const bool shouldEmitCompletion = true;
    action->AddAction(placeAction, false, shouldEmitCompletion);
  }

  {  
    static constexpr float kBackUpMinMM = 40.0;
    static constexpr float kBackUpMaxMM = 70.0;
    double backup_amount = robot.GetRNG().RandDblInRange(kBackUpMinMM,kBackUpMaxMM);
    
    action->AddAction( new DriveStraightAction(robot, -backup_amount, DEFAULT_PATH_MOTION_PROFILE.speed_mmps) );
  }

  StartActing(action, [this](){ BehaviorObjectiveAchieved(BehaviorObjective::PickedUpAndPutDownBlock); });
}



}
}



