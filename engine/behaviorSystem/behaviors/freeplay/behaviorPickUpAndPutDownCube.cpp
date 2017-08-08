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

#include "engine/behaviorSystem/behaviors/freeplay/behaviorPickUpAndPutDownCube.h"

#include "engine/actions/basicActions.h"
#include "engine/actions/dockActions.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"
#include "engine/components/carryingComponent.h"
#include "engine/robot.h"

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
  if(robot.GetCarryingComponent().IsCarryingObject()){
    _targetBlockID = robot.GetCarryingComponent().GetCarryingObject();
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

  StartActing(action, [this]()
                      {
                        BehaviorObjectiveAchieved(BehaviorObjective::PickedUpAndPutDownBlock);
                        NeedActionCompleted();
                      });
}



}
}



