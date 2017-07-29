/**
 * File: behaviorPlaypenDriveForwards.cpp
 *
 * Author: Al Chaussee
 * Created: 07/28/17
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/behaviors/devBehaviors/playpen/behaviorPlaypenDriveForwards.h"

#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
//#include "anki/cozmo/basestation/factory/factoryTestLogger.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

namespace {
//static const f32 kDistanceToTriggerFrontCliffs_mm = 20;
//static const f32 kDistanceToTriggerBackCliffs_mm = 20;
}

BehaviorPlaypenDriveForwards::BehaviorPlaypenDriveForwards(Robot& robot, const Json::Value& config)
: IBehaviorPlaypen(robot, config)
{
  SubscribeToTags({{EngineToGameTag::RobotStopped,
                    EngineToGameTag::CliffEvent}});
}

void BehaviorPlaypenDriveForwards::GetResultsInternal()
{
  
}

Result BehaviorPlaypenDriveForwards::InternalInitInternal(Robot& robot)
{
  robot.GetRobotMessageHandler()->SendMessage(robot.GetID(),
    RobotInterface::EngineToRobot(RobotInterface::EnableStopOnCliff(false)));

  MoveHeadToAngleAction* headToZero = new MoveHeadToAngleAction(robot, 0);
  MoveLiftToHeightAction* liftDown = new MoveLiftToHeightAction(robot, LIFT_HEIGHT_LOWDOCK);
  CompoundActionParallel* action = new CompoundActionParallel(robot, {headToZero, liftDown});
  
  
  
  StartActing(action, [this, &robot](){
    BlockWorldFilter filter;
    filter.SetFilterFcn(nullptr);
    filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
    robot.GetBlockWorld().DeleteLocatedObjects(filter);
    
    TransitionToWaitingForFrontCliffs(robot);
  });
  
  
  return RESULT_OK;
}

BehaviorStatus BehaviorPlaypenDriveForwards::InternalUpdateInternal(Robot& robot)
{

  return BehaviorStatus::Running;
}

void BehaviorPlaypenDriveForwards::StopInternal(Robot& robot)
{
  
}

void BehaviorPlaypenDriveForwards::TransitionToWaitingForFrontCliffs(Robot& robot)
{
  
}

void BehaviorPlaypenDriveForwards::HandleWhileRunningInternal(const EngineToGameEvent& event, Robot& robot)
{
  const EngineToGameTag tag = event.GetData().GetTag();
  if(tag == EngineToGameTag::CliffEvent)
  {
//    const auto& payload = event.GetData().Get_CliffEvent();
//    
//    const uint8_t FL = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FL));
//    const uint8_t FR = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_FR));
//    const uint8_t BL = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_BL));
//    const uint8_t BR = (1<<Util::EnumToUnderlying(CliffSensor::CLIFF_BR));
    
//    payload.detectedFlags
  }
}

}
}


